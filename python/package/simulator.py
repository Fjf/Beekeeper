import random
from typing import Optional

import torch

from MCTS import HiveMCTS
from hive import Hive
from hive_nn import HiveNN
from utils import remove_tile_idx, HiveState


class HiveSimulator:
    def __init__(self, mcts_iterations=100):
        self.do_mcts = True
        self.mcts_object = HiveMCTS(mcts_iterations)

    @staticmethod
    def select_move(policy, filter_idx=None, best=False):
        """
        Select a move based on the provided policy vector only selecting moves according to the provided filter vector.
        The return value is  0 <= return < len(filter_idx)

        :param best:
        :param policy: a vector with move probabilities
        :param filter_idx: a vector defining the moves from which to choose
        :return: the index of the chosen move in the filter vector
        """
        if len(filter_idx) == 1:
            return 0
        if sum(policy[filter_idx] == 0):  # Select random move if policy sums to 0
            # logging.getLogger("hive")\
            #     .warning("Filtered policy summed to 0, selecting random move. Training is probably going wrong.")
            return random.choice(range(len(filter_idx)))

        # Pick the best move or a random one based on policy probabilities
        if best:
            return torch.argmax(policy[filter_idx].flatten())
        else:
            return torch.multinomial(policy[filter_idx].flatten(), 1)[0]

    def nn_move(self, network, hive, mcts=True):
        """
          Tile format (8 bits);
            [ NN C TTTTT ]

        Selects a move based on the policy vector and updates the board state.
        Returns the policy vector with which the move was chosen.

        :param network: the neural network for which to compute the move
        :param hive: the hive state for which to compute the move
        :param mcts: use mcts to compute an improvement policy?
        :param p1: is this player 1? if not invert the policy.
        :return: the taken move index in the policy vector
        """
        # Fetch array from internal memory and remove the tile number.
        arr = hive.node.contents.to_np()
        arr = remove_tile_idx(arr)
        arr = arr.reshape(1, 5, 26, 26)

        # Convert data to tensorflow usable format
        data = torch.Tensor(arr)

        # Get tensor of preferred outputs by priority.
        policy, _ = network(data)
        policy = policy[0]

        # Compute updated MCTS policy
        if mcts:
            updated_policy = self.mcts_object.process(hive.node, policy, network)
        else:
            updated_policy = policy

        # Convert children to usable format.
        children = list(hive.children())
        encodings = [child.encode() for child in children]

        assert (len(children) > 0)

        # Select only best moves after high enough temperature
        temperature = hive.turn() / hive.turn_limit

        # Invert choice for move selection if you are player 2
        if hive.turn() % 2 != 0:
            updated_policy = 1 - updated_policy

        # Select a move based on updated_policy's weights out of the valid moves (encodings)
        best_idx = self.select_move(updated_policy, filter_idx=encodings, best=temperature > 0.5)

        hive.select_child(children[best_idx], n=best_idx)
        return encodings[best_idx]

    def play(self, hive, p1: HiveNN, p2: Optional[HiveNN]):
        """
        Does a playout of the game with two neural networks, returns all board states and the final result.
        The returned zip contains all boards which were played by player 1 including the policy vector.
        This can later be used to train the neural network.

        :param hive: the board state from which to choose
        :param p1: the neural network for player 1
        :param p2: the neural network for player 2, if None, use random agent instead
        :return: zip(boards, policies), result
        """
        indices = []
        # Play game until a terminal state has occurred.
        res = HiveState.UNDETERMINED
        for i in range(hive.turn_limit):
            if i % 2 == 0:
                idx = self.nn_move(p1, hive, mcts=True)
                indices.append(idx)
            else:
                # This is not the learning neural network, so don't track these boards.
                hive.track_history = False
                if p2 is None:
                    hive.ai_move(algorithm="mm")
                else:
                    self.nn_move(p2, hive, mcts=True)
                hive.track_history = True

            res = hive.finished()

            if res != HiveState.UNDETERMINED:
                break

        # Return all board states and the final result.
        return zip(hive.history, indices), res

    def parallel_play(self, task_idx: int, net1: HiveNN, net2: Optional[HiveNN] = None):
        """
        Wrapper for the play function preparing the data and initializing the Hive object.
        It will convert the arrays to tensors to be used by pytorch.
        This function is threadsafe.

        :param task_idx:
        :param net1: player 1's neural network
        :param net2: optional player 2's neural network, or played by a random agent is it is None
        :return: tensors, targets, and the result of the game.
        """

        tensors = []
        taken_moves = []

        hive = Hive(track_history=True)

        node_result, result = self.play(hive, net1, net2)

        for board_idx, (node, move) in enumerate(node_result):
            # Fetch array from internal memory and remove the tile number.
            arr = remove_tile_idx(node.contents.to_np())

            tensors.append(torch.Tensor(arr))
            taken_moves.append(move)

        # TODO: Wrap this in a namedtuple
        return tensors, taken_moves, result

