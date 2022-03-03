import copy
import logging
import os
import pickle
from datetime import datetime
from typing import Type

import numpy as np
import pytorch_lightning
import torch
from mpi4py import MPI
from pytorch_lightning import Trainer
from torch.utils.data import DataLoader
from tqdm import tqdm

from games.Game import Game
from games.hive.hive import GameState
from games.hive.hive_dataset import HiveDataset
from games.utils import Perspectives
from simulator import Simulator


class School:
    def __init__(self, game: Type[Game], network: Type[pytorch_lightning.LightningModule], simulations=100,
                 n_old_data=1):
        self.logger = logging.getLogger("Hive")
        self.network_type = network

        self.data_dir = "data/"
        self.model_dir = ""

        self.pretraining = False

        self.game = game
        self.simulator = Simulator(game, 40)

        self.logger.info(f"Initializing training and stable network ({game.input_space} -> {game.action_space}).")
        self.updating_network = network(game.input_space, game.action_space)
        self.stable_network = copy.deepcopy(self.updating_network)

        self.simulations = simulations

        self.n_old_data = n_old_data
        self.old_data_storage = []
        self.comm = MPI.COMM_WORLD
        self.logger.info("Done initializing trainer.")

    def generate_data(self):
        # If you are pretraining, play against the fixed opponent instead.
        if self.pretraining:
            players = (self.updating_network, None)
        else:
            players = (self.updating_network, self.stable_network)

        self.logger.info(f"Simulating {self.simulations} games on {self.comm.Get_size()} threads.")
        start = datetime.now()

        # Broadcast latest models
        self.comm.bcast(pickle.dumps(players))

        # Gather data from all workers
        data = []
        print("Gathering data")
        for _ in tqdm(range(self.simulations)):
            data.append(self.comm.recv())
        self.logger.debug(f"Spent {datetime.now() - start} to simulate {self.simulations} games.")
        return data

    @staticmethod
    def get_win(result: GameState, perspective):
        if (result == GameState.WHITE_WON and perspective == Perspectives.PLAYER1) \
                or (result == GameState.BLACK_WON and perspective == Perspectives.PLAYER2):
            return 1
        if (result == GameState.BLACK_WON and perspective == Perspectives.PLAYER1) \
                or (result == GameState.WHITE_WON and perspective == Perspectives.PLAYER2):
            return 0
        return 0.5

    def winrate(self, p1, p2, n_games=100):
        wins = 0
        self.simulator.temperature_threshold = 0
        for i in range(n_games):
            perspective = Perspectives.PLAYER1 if i % 2 == 0 else Perspectives.PLAYER2
            game = self.game()

            if perspective == Perspectives.PLAYER1:
                _, result = self.simulator.play(game, p1, p2, mcts=False)
            else:
                _, result = self.simulator.play(game, p2, p1, mcts=False)

            wins += self.get_win(result, perspective)

        return wins / n_games

    def train(self, updates=100, pretraining=False, stored_model_filename: str = None):
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

        if stored_model_filename is not None:
            self.logger.info(f"Loading model '{stored_model_filename}'")

            self.updating_network = self.network_type()
            self.updating_network.load_state_dict(torch.load(stored_model_filename))

            self.stable_network = copy.deepcopy(self.updating_network)

        network_iter = 0

        for u in range(updates):
            filename = os.path.join(self.data_dir, f"example{u}.data")
            if not os.path.isfile(filename):
                data = self.generate_data()

                # Combine the results of each thread into single packet arrays.
                tensors = []
                policy_vectors = []
                outcomes = []

                arr = np.array([i for _, _, _, i, _ in data])
                arr = (arr + 1) / 2
                print(f"Data generation WR: {sum(arr) / len(arr)}")

                for i, tensor, policy_vector, result, perspective in data:
                    tensors += tensor
                    policy_vectors += policy_vector
                    outcomes += [torch.Tensor([result])] * len(tensor)

                torch.save(list(zip(tensors, policy_vectors, outcomes)), filename)
            else:
                tensors, policy_vectors, outcomes = zip(*torch.load(filename))
                tensors = list(tensors)
                policy_vectors = list(policy_vectors)
                outcomes = list(outcomes)
                print(len(tensors), len(policy_vectors), len(outcomes))

            self.logger.info(f"{len(tensors)} boards added to dataset.")
            if len(tensors) == 0:
                self.logger.warning("No tensors added, re-running simulation to get more data.")
                continue

            # Initialize dataset for training loop
            dataset = HiveDataset(tensors, policy_vectors, outcomes)

            # Store older data for N updates
            self.old_data_storage.append(dataset)
            if len(self.old_data_storage) > self.n_old_data:
                self.old_data_storage = self.old_data_storage[1:]

            aggregated_dataset = HiveDataset([], [], [])
            for old_data in self.old_data_storage:
                aggregated_dataset += old_data

            self.logger.info(f"Training on {len(aggregated_dataset)} boards.")
            dataloader = DataLoader(aggregated_dataset, batch_size=16, shuffle=True, num_workers=0)

            trainer = Trainer(gpus=1, max_epochs=30, default_root_dir="checkpoints")
            trainer.fit(self.updating_network, dataloader)

            # print("#######################################################\n"
            #       "# Board states - Policy vector - Perspective - Outcome\n"
            #       "#######################################################\n")
            # for i in range(20):
            #     board = dataset.boards[i][:35]
            #     board = torch.reshape(board, (5, 7))
            #     print(np.rot90())
            #     print(dataset.expected[i], dataset.outcomes[i].item())
            #     pi, v = self.updating_network(dataset.boards[i].reshape(1, *dataset.boards[i].shape))
            #     print(pi, v)

            winrate = self.winrate(self.updating_network, self.stable_network, n_games=200)
            self.logger.info(f"Current performance: {winrate * 100}% wr")
            if winrate > 0.55:
                self.logger.info(f"Updating network, iteration {network_iter}.")
                torch.save(self.updating_network.state_dict(), os.path.join(self.model_dir, f"iteration_{network_iter}.pt"))
                self.stable_network.load_state_dict(self.updating_network.state_dict())
                network_iter += 1

            # torch.cuda.empty_cache()
