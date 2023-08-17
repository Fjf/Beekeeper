import copy
import logging
import os
import pickle
from collections import defaultdict
from datetime import datetime
from typing import Type

import numpy as np
import pytorch_lightning
import torch
from mpi4py import MPI
from pytorch_lightning import Trainer
from torch.utils.data import DataLoader
from tqdm import tqdm

from games.Game import Game, GameNode
from games.hive.hive import GameState
from games.hive.hive_dataset import HiveDataset
from games.utils import Perspectives
from mpi_packet import MPIPacketState, MPIPacket
from simulator import Simulator


class School:
    def __init__(self, game: Type[Game], network: Type[pytorch_lightning.LightningModule], n_sims=100,
                 n_data_reuse=1, model_dir="model", data_dir="data", device="cuda:0", **kwargs):
        self.logger = logging.getLogger("Hive")
        self.network_type = network

        self.data_dir = data_dir
        self.model_dir = model_dir

        # Create directories
        if self.data_dir is not None:
            os.makedirs(self.data_dir, exist_ok=True)
        if self.model_dir is not None:
            os.makedirs(self.model_dir, exist_ok=True)

        self.pretraining = False

        self.game = game
        self.simulator = Simulator(game, 40, device=device)

        self.logger.info(f"Initializing training and stable network ({game.input_space} -> {game.action_space}).")
        self.updating_network = network(game.input_space, game.action_space)
        self.stable_network = copy.deepcopy(self.updating_network)

        self.simulations = n_sims

        self.n_old_data = n_data_reuse
        self.old_data_storage = []
        self.comm = MPI.COMM_WORLD
        self.logger.info("Done initializing trainer.")

    def generate_data(self):
        # If you are pretraining, play against the fixed opponent instead.
        if self.pretraining:
            players = (self.updating_network.to("cpu").state_dict(), None)
        else:
            players = (self.updating_network.to("cpu").state_dict(), self.stable_network.to("cpu").state_dict())

        self.logger.info(f"Simulating {self.simulations} games on {self.comm.Get_size()} threads.")
        start = datetime.now()

        # Create packet to send.
        packet = MPIPacket(MPIPacketState.PROCESS, players)

        # Broadcast latest models' state dicts
        self.comm.bcast(pickle.dumps(packet))

        # Gather data from all workers
        data = []
        for _ in tqdm(range(self.simulations)):
            i, tensors, policy_vectors, result_values, nn_perspective = self.comm.recv()
            # if result_values[0] == 0:
            #     continue
            data.append((i, tensors, policy_vectors, result_values, nn_perspective))
        self.logger.debug(f"Spent {datetime.now() - start} to simulate {self.simulations} games.")

        return data

    @staticmethod
    def get_win(result: GameState, perspective: Perspectives) -> float:
        """
        Given a GameState and perspective, it will give a value from 0, 0.5, and 1, for a loss, draw, or win.
        :param result: the final gamestate
        :param perspective: the perspective for which to compute the game value
        :return: the converted game value.
        """
        if (result == GameState.WHITE_WON and perspective == Perspectives.PLAYER1) \
                or (result == GameState.BLACK_WON and perspective == Perspectives.PLAYER2):
            return 1
        if (result == GameState.BLACK_WON and perspective == Perspectives.PLAYER1) \
                or (result == GameState.WHITE_WON and perspective == Perspectives.PLAYER2):
            return 0
        return 0.5

    def winrate(self, p1, p2, n_games=100):
        """
        Compute winrate of two models against each other, with half games played from either perspective.
        MCTS is turned off for winrate computation, meaning only the raw neural network policy values are used.

        :param p1: the main model
        :param p2: the opposing model
        :param n_games: the amount of games to base the winrate on
        :return: float of winrate, a draw counts as 0.5 wins.
        """
        results = [0, 0, 0]
        wins = 0
        self.simulator.temperature_threshold = 0
        p1 = p1.to("cuda")
        p2 = p2.to("cuda")
        for i in tqdm(range(n_games)):
            perspective = Perspectives.PLAYER1 if i % 2 == 0 else Perspectives.PLAYER2
            game = self.game()

            if perspective == Perspectives.PLAYER1:
                boards, result = self.simulator.play(game, p1, p2, mcts=False)

                if i + 2 >= n_games:
                    game = self.game()
                    boards2, result2 = self.simulator.play(game, p1, p2, mcts=True)
            else:
                boards, result = self.simulator.play(game, p2, p1, mcts=False)

                if i + 2 >= n_games:
                    game = self.game()
                    boards2, result2 = self.simulator.play(game, p2, p1, mcts=True)

            if i + 2 >= n_games:
                # print("Node data")
                # for (node, policy) in boards:
                #     print(node)
                #     print(policy.cpu().numpy().reshape(3, 3))

                print("\n\n##############\nNode data2")
                for (node, policy) in boards2:
                    node: GameNode
                    print(node)
                    print(node.mcts.policy)
                    print(policy.cpu().numpy())
                    print(node.mcts.value / node.mcts.n_sims, result2)
                    # asd = np.zeros(9)
                    # for child in node.children:
                    #     asd[child.encode()] = child.mcts.n_sims
                    # print([child.mcts.n_sims for child in node.children])

            win = self.get_win(result, perspective)
            wins += win
            results[int(win * 2)] += 1

        p1 = p1.to("cpu")
        p2 = p2.to("cpu")

        return wins / n_games, results

    def train(self, updates=100, pretraining=False, stored_model_filename: str = None, batch_size=256, **kwargs):
        """
        The reinforcement learning loop.

        :param updates: amount of replacements for the network to do until termination
        :param pretraining: do pre-training against a fixed opponent before reinforcement
        :param stored_model_filename: if not None, fetch weights from disk
        :return:
        """
        torch.manual_seed(1234)

        self.pretraining = pretraining
        assert (not self.pretraining), "Pretraining is not yet implemented."

        ##############################################################################
        # Load existing model from disk as a checkpoint to start from.
        ##############################################################################
        if stored_model_filename is not None:
            self.logger.info(f"Loading model '{stored_model_filename}'")

            self.updating_network = self.network_type()
            self.updating_network.load_state_dict(torch.load(stored_model_filename))

            self.stable_network = copy.deepcopy(self.updating_network)

        network_iter = 0

        for u in range(updates):
            if self.data_dir is not None:
                filename = os.path.join(self.data_dir, f"example{u}.data")
            else:
                filename = None

            if filename is None or not os.path.isfile(filename):
                ##############################################################################
                # Generate data with current two models
                ##############################################################################
                data = self.generate_data()
                # Combine the results of each thread into single packet arrays.
                tensors = []
                policy_vectors = []
                outcomes = []

                for i, tensor, policy_vector, result, perspective in data:
                    tensors += tensor
                    policy_vectors += policy_vector
                    outcomes += result

                if filename is not None:
                    torch.save(list(zip(tensors, policy_vectors, outcomes)), filename)
            else:
                ##############################################################################
                # Load existing data from disk.
                ##############################################################################
                tensors, policy_vectors, outcomes = zip(*torch.load(filename))

            # Convert to tensors
            if len(tensors) > 0:
                tensors = torch.stack(tensors)
                policy_vectors = torch.stack(policy_vectors)
                outcomes = torch.stack(outcomes)

            out_res = defaultdict(int)
            for outcome in outcomes:
                out_res[int(outcome.item())] += 1
            self.logger.info(f"Training winrate: {out_res}")
            self.logger.info(f"{len(tensors)} boards added to dataset.")
            if len(tensors) == 0:
                self.logger.warning("No tensors added, re-running simulation to get more data.")
                continue

            ##############################################################################
            # Aggregate data into Dataset
            ##############################################################################
            dataset = HiveDataset(tensors, policy_vectors, outcomes, cuda=False)

            # Store older data for N updates
            self.old_data_storage.append(dataset)
            if len(self.old_data_storage) > self.n_old_data:
                self.old_data_storage = self.old_data_storage[1:]

            aggregated_dataset = copy.deepcopy(self.old_data_storage[0])
            for old_data in self.old_data_storage[1:]:
                aggregated_dataset += old_data

            self.logger.info(f"Training on {len(aggregated_dataset)} boards.")
            dataloader = DataLoader(aggregated_dataset, batch_size=batch_size, shuffle=True, num_workers=0)

            ##############################################################################
            # Train model, then check if model is good enough to replace existing model
            ##############################################################################
            self.updating_network.train()
            trainer = Trainer(accelerator="gpu", devices=1, precision=16, max_epochs=10,
                              default_root_dir="checkpoints")
            trainer.fit(self.updating_network, dataloader)
            self.updating_network.eval()

            with torch.no_grad():
                winrate, results = self.winrate(self.updating_network, self.stable_network, n_games=200)
                self.logger.info(f"Current performance: {winrate * 100}% wr ({results})")
                if winrate > 0.6:
                    self.logger.info(f"Updating network, iteration {network_iter}.")
                    torch.save(self.updating_network.state_dict(),
                               os.path.join(self.model_dir, f"iteration_{network_iter}.pt"))
                    self.stable_network.load_state_dict(self.updating_network.state_dict())
                    network_iter += 1
