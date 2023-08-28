"""
Monte Carlo Tree Search (MCTS)

This class will do neural-network augmented MCTS simulations, providing an improvement over the policy vector.
In the current state, the improved policy vector generates all available moves from the current board-state.
For every board it will:
 - Check if it is a terminal state
   - true:  set child_value to 0 for a loss, or 1 for a win.
   - false: retrieve the (child_policy, child_value) vector from the neural network
 - Compute policy vector index i
 - Set policy[i] to the newly retrieved child_value.
"""
import math
import time
from typing import Tuple, Optional, List

import numpy as np
import torch
from torch import nn

from games.Game import Game, GameNode


class MCTS:
    def __init__(self, iterations=100, device="cpu", mcts_batch_size=16, exploration_factor=0.6):
        self.iterations = iterations
        self.device = torch.device(device)
        self.batch_size = mcts_batch_size
        self.exploration_factor = exploration_factor

    def ucb(self, child, parent):
        if child.mcts.n_sims > 0:
            value = (
                    (child.mcts.value / child.mcts.n_sims) +
                    self.exploration_factor *
                    child.mcts.prior *
                    math.sqrt(parent.mcts.n_sims) /
                    (1 + child.mcts.n_sims)
            )
        else:
            value = (
                    self.exploration_factor *
                    child.mcts.prior *
                    math.sqrt(parent.mcts.n_sims + 1e-5)
            )

        return value

    def search(self, node: GameNode, network):
        if node.winner() is not None:
            if node.winner() == node.to_play:
                win_value = 1.
            elif node.winner() == -1:
                win_value = 0.
            else:
                win_value = -1.

            return -win_value

        if len(node.children) == 0:
            node.expand()

            # Predict next move
            policy, value = network.predict(node.to_np(node.to_play))
            # Only valid moves should be non-zero
            valid_indices = torch.IntTensor([child.encode() for child in node.children])
            np_policy = np.zeros(policy.shape)
            np_policy[valid_indices] = torch.index_select(policy, 0, valid_indices).detach()

            policy_sum = np.sum(np_policy)
            if policy_sum != 0:
                node.mcts.policy = np_policy / policy_sum
            else:
                raise ValueError("Valid policy summed to 0.")

            return -value.item()

        best, best_value = None, -float("inf")
        for child in node.children:
            ucb = self.ucb(child, node)
            if ucb > best_value:
                best, best_value = child, ucb

        value = self.search(best, network)
        assert type(value) == float
        if best.mcts.value is None:
            best.mcts.value = value
            best.mcts.n_sims = 1
        else:
            best.mcts.value += value
            best.mcts.n_sims += 1

        node.mcts.n_sims += 1
        return -value

    def process(self, game: Game, network, temperature=1.):
        """
        Processes a node with a policy and returns the MCTS-augmented policy.
        :param p1:
        :param network:
        :param game:
        :return:
        """
        game.reset_children()
        game.node.mcts.value = 0

        for i in range(self.iterations + 1):
            game.node.mcts.value += self.search(game.node, network)
            # print( self.create_policy_vector(game, temperature))

            game.node.mcts.n_sims += 1
        return self.create_policy_vector(game, temperature)

    def create_policy_vector(self, game, temperature):
        counts = torch.zeros(game.node.mcts.policy.shape)
        for child in game.children():
            counts[child.encode()] += child.mcts.n_sims

        if temperature == 0:
            best_counts = torch.argwhere(counts == torch.max(counts)).flatten()
            best = np.random.choice(best_counts)
            policy_vector = torch.zeros(game.node.mcts.policy.shape)
            policy_vector[best] = 1
            return policy_vector

        counts = counts ** (1 / temperature)
        policy_vector = counts / torch.sum(counts)
        return policy_vector
