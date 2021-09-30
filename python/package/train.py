import copy
import logging
import random
from collections import namedtuple, defaultdict
from datetime import datetime
from typing import Optional

import numpy as numpy
import torch
from joblib import delayed, Parallel
from pytorch_lightning import Trainer
from torch.utils.data import DataLoader

from MCTS import HiveMCTS
from hive import Hive, HiveState
from hive_dataset import HiveDataset
from hive_nn import HiveNN
from utils import remove_tile_idx


def select_move(policy, filter_idx=None):
    """
    Select a move based on the provided policy vector only selecting moves according to the provided filter vector.
    The return value is  0 <= return < len(filter_idx)

    :param policy: a vector with move probabilities
    :param filter_idx: a vector defining the moves from which to choose
    :return: the index of the chosen move in the filter vector
    """
    if len(filter_idx) == 1:
        return 0

    # TODO: Better activation function.
    policy_idx = torch.multinomial(policy[filter_idx].flatten(), 1)
    return policy_idx[0]


def nn_move(network, hive, mcts=True, p1=True):
    """
      Tile format (8 bits);
        [ NN C TTTTT ]

    Selects a move based on the policy vector and updates the board state.
    Returns the policy vector with which the move was chosen.

    :param network: the neural network for which to compute the move
    :param hive: the hive state for which to compute the move
    :param mcts: use mcts to compute an improvement policy?
    :param p1: is this player 1? if not invert the policy.
    :return: the policy vector for this board.
    """
    # Fetch array from internal memory and remove the tile number.
    arr = hive.node.contents.to_np()
    arr = remove_tile_idx(arr)
    arr = arr.reshape(1, 5, 26, 26)

    # Convert data to tensorflow usable format
    data = torch.Tensor(arr)

    # Get tensor of preferred outputs by priority.
    policy, _ = network(data)

    # Compute updated MCTS policy
    if mcts:
        updated_policy = HiveMCTS.process(hive.node, policy, network)

        # Invert the policy if this is player 2.
        if not p1:
            updated_policy = 1 - updated_policy
    else:
        updated_policy = policy

    # Convert children to usable format.
    children = list(hive.children())
    encodings = [child.encode() for child in children]

    assert (len(children) > 0)

    # Select a move based on updated_policy's weights out of the valid moves (encodings)
    best_idx = select_move(updated_policy, filter_idx=encodings)

    hive.select_child(children[best_idx], n=best_idx)
    return updated_policy


def play(hive, p1: HiveNN, p2: Optional[HiveNN]):
    """
    Does a playout of the game with two neural networks, returns all board states and the final result.
    The returned zip contains all boards which were played by player 1 including the policy vector.
    This can later be used to train the neural network.

    :param hive: the board state from which to choose
    :param p1: the neural network for player 1
    :param p2: the neural network for player 2, if None, use random agent instead
    :return: zip(boards, policies), result
    """
    policies = []

    # Play game until a terminal state has occurred.
    res = HiveState.UNDETERMINED
    for i in range(hive.turn_limit):
        if i % 2 == 0:
            policy = nn_move(p1, hive, p1=True, mcts=True)
            policies.append(policy)
        else:
            # This is not the learning neural network, so don't track these boards.
            hive.track_history = False
            if p2 is None:
                hive.ai_move(algorithm="random")
            else:
                nn_move(p2, hive, p1=False, mcts=False)
            hive.track_history = True

        res = hive.finished()
        if res != HiveState.UNDETERMINED:
            break

    # Return all board states and the final result.
    return zip(hive.history, policies), res


def parallel_play(net1, net2=None):
    """
    Wrapper for the play function preparing the data and initializing the Hive object.
    It will convert the arrays to tensors to be used by pytorch.
    This function is threadsafe.

    :param net1: player 1's neural network
    :param net2: optional player 2's neural network, or played by a random agent is it is None
    :return: tensors, targets, and the result of the game.
    """
    tensors = []
    targets = []

    hive = Hive(track_history=True)
    node_result, result = play(hive, net1, net2)

    # Don't add draws to the data to train from?
    # if result != HiveState.WHITE_WON and result != HiveState.BLACK_WON:
    #     return [], [], result

    for board_idx, (node, policy) in enumerate(node_result):
        # Fetch array from internal memory and remove the tile number.
        arr = remove_tile_idx(node.contents.to_np())

        tensors.append(torch.Tensor(arr))
        targets.append(policy)

    return tensors, targets, result


class School:
    def __init__(self):
        self.pretraining = False
        self.logger = logging.getLogger("Hive")
        nn_input_size = 26 * 26  # Board size is 26x26
        nn_action_space = 11 * 22 * 7  # 11 tiles can go next to 22 tiles in 7 directions

        self.updating_network = HiveNN(nn_input_size, nn_action_space)
        self.stable_network = HiveNN(nn_input_size, nn_action_space)

        self.policies = []  # Stores policy tensors

        self.n_old_data = 1
        self.old_data_storage = []

    def train(self, updates=100, simulations=300, pretraining=True, stored_board: str = None):
        """
        The reinforcement learning loop.

        :param updates: amount of replacements for the network to do until termination
        :param simulations: amount of simulations per iteration of the network
        :param pretraining: do pre-training against a fixed opponent before reinforcement
        :param stored_board: if not None, fetch weights from disk
        :return:
        """
        self.pretraining = pretraining
        if stored_board is not None:
            self.updating_network = torch.load(stored_board)

        for u in range(updates):
            import os
            jobs = os.cpu_count()

            self.logger.info(f"Simulating {simulations} games on {jobs} threads.")
            start = datetime.now()

            # If you are pretraining, play against the fixed opponent instead.
            if self.pretraining:
                players = (self.updating_network, None)
            else:
                players = (self.updating_network, self.stable_network)

            # Do N playout games in parallel using as many cores as there are available.
            data = Parallel(n_jobs=jobs)(
                delayed(parallel_play)(*players) for _ in range(simulations))
            self.logger.debug(f"Spent {datetime.now() - start} to simulate {simulations} games.")

            # Combine the results of each thread into single packet arrays.
            tensors = []
            targets = []
            results = defaultdict(int)
            for tensor, target, result in data:
                tensors += tensor
                targets += target
                results[result] += 1

            # Pretty print the results of the simulated games.
            result_formatted = "\n\t".join(f"{k}: {v}" for k, v in results.items())
            winrate = results[HiveState.WHITE_WON] / sum(results[state] for state in HiveState)
            self.logger.info(f"Result of 100 runs: {result_formatted} ({winrate}).")

            # When encountering a 90% winrate during pretraining the actual reinforcement learning can start.
            if pretraining and winrate > 0.9:
                pretraining = False

            if not pretraining and winrate > 0.6:
                self.stable_network = copy.deepcopy(self.updating_network)

            # tensors, targets = parallel_play(self.updating_network, self.stable_network)
            self.logger.info(f"{len(tensors)} boards added to dataset.")
            if len(tensors) == 0:
                self.logger.warning("No tensors added, re-running simulation to get more data.")
                continue

            # Initialize dataset for training loop
            dataset = HiveDataset(tensors, targets)

            # Store older data for N updates
            self.old_data_storage.append(dataset)
            if len(self.old_data_storage) > self.n_old_data:
                self.old_data_storage = self.old_data_storage[1:]

            aggregated_dataset = HiveDataset([], [])
            for data in self.old_data_storage:
                aggregated_dataset += data

            self.logger.info(f"Training on {len(aggregated_dataset)} boards.")
            dataloader = DataLoader(aggregated_dataset)

            trainer = Trainer(gpus=1, max_epochs=10, default_root_dir="checkpoints")
            trainer.fit(self.updating_network, dataloader)

            torch.save(self.updating_network, "tensor.pt")

    @staticmethod
    def test(p1, p2):
        hive = Hive()

        results = defaultdict(int)

        n_samples = 100
        for i in range(n_samples):
            hive.reinitialize()

            res = HiveState.UNDETERMINED
            # Play for N turns
            for t in range(hive.turn_limit - 1):
                if i % 2 == 0:
                    nn_move(p1, hive, mcts=True, p1=True)
                else:
                    hive.ai_move("random")
                    # nn_move(p2, hive, mcts=False, p1=False)
                res = hive.finished()
                if res != HiveState.UNDETERMINED:
                    break

            results[res] += 1

        return results
