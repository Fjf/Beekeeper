import cProfile
import copy
import logging
import pickle
import random
from collections import defaultdict
import os
from datetime import datetime
from typing import Optional

import torch
from joblib import delayed, Parallel
from mpi4py import MPI
from pytorch_lightning import Trainer
from torch.utils.data import DataLoader

from MCTS import HiveMCTS
from hive import Hive, HiveState, lib
from hive_dataset import HiveDataset
from hive_nn import HiveNN
from simulator import HiveSimulator
from utils import remove_tile_idx


class School:
    def __init__(self, simulations=100):
        self.pretraining = False
        self.logger = logging.getLogger("Hive")
        nn_input_size = 26 * 26  # Board size is 26x26
        nn_action_space = 11 * 22 * 7  # 11 tiles can go next to 22 tiles in 7 directions

        self.updating_network = HiveNN(nn_input_size, nn_action_space)
        self.stable_network = HiveNN(nn_input_size, nn_action_space)

        self.simulations = simulations

        self.n_old_data = 10
        self.old_data_storage = []
        self.comm = MPI.COMM_WORLD

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
        for i in range(self.simulations):
            data.append(self.comm.recv())

        self.logger.debug(f"Spent {datetime.now() - start} to simulate {self.simulations} games.")
        return data

    def train(self, updates=100, pretraining=False, stored_model_filename: str = None):
        """
        The reinforcement learning loop.

        :param updates: amount of replacements for the network to do until termination
        :param pretraining: do pre-training against a fixed opponent before reinforcement
        :param stored_model_filename: if not None, fetch weights from disk
        :return:
        """
        self.pretraining = pretraining
        if stored_model_filename is not None:
            self.updating_network = torch.load(stored_model_filename)

        for u in range(updates):
            data = self.generate_data()

            # Combine the results of each thread into single packet arrays.
            # TODO: Refactor
            tensors = []
            taken_moves = []
            outcomes = []

            results = defaultdict(int)
            for tensor, moves, result in data:
                tensors += tensor
                taken_moves += moves

                # TODO: refactor
                if result == HiveState.WHITE_WON:
                    res = 1.
                elif result == HiveState.BLACK_WON:
                    res = 0.
                else:
                    res = 0.5

                outcomes += [torch.Tensor([res])] * len(tensor)
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

            self.logger.info(f"{len(tensors)} boards added to dataset.")
            if len(tensors) == 0:
                self.logger.warning("No tensors added, re-running simulation to get more data.")
                continue

            # Initialize dataset for training loop
            dataset = HiveDataset(tensors, taken_moves, outcomes)

            # Store older data for N updates
            self.old_data_storage.append(dataset)
            if len(self.old_data_storage) > self.n_old_data:
                self.old_data_storage = self.old_data_storage[1:]

            aggregated_dataset = HiveDataset([], [], [])
            for data in self.old_data_storage:
                aggregated_dataset += data

            self.logger.info(f"Training on {len(aggregated_dataset)} boards.")
            dataloader = DataLoader(aggregated_dataset, batch_size=64)

            trainer = Trainer(gpus=1, max_epochs=10, default_root_dir="checkpoints")
            trainer.fit(self.updating_network, dataloader)

            # torch.cuda.empty_cache()
            # torch.save(self.updating_network, "tensor.pt")

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
                    HiveSimulator.nn_move(p1, hive, mcts=True)
                else:
                    hive.ai_move("random")
                    # nn_move(p2, hive, mcts=False, p1=False)
                res = hive.finished()
                if res != HiveState.UNDETERMINED:
                    break

            results[res] += 1

        return results
