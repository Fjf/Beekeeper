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

from games.Game import Game, GameNode
from games.utils import GameState, Perspectives


class MCTS:
    def __init__(self, iterations=100, device="cpu", mcts_batch_size=16):
        self.iterations = iterations
        self.mcts_constant = 1
        self.device = torch.device(device)
        self.batch_size = mcts_batch_size

    def select_best(self, root: GameNode) -> GameNode:
        parent = root

        # While we are not done, and we have expanded the node, search deeper
        while parent.finished() is not None and len(parent.children) > 0:
            children = parent.children
            best_value, best = -float("inf"), None
            for i, child in enumerate(children):
                value = self.ucb(child, parent)
                if best_value < value:
                    best_value, best = value, child

            assert best is not None
            parent = best
        return parent

    @staticmethod
    def ucb(child, parent):
        ef = 1  # exploration factor
        prior_score = child.mcts.prior * ef * math.sqrt(parent.mcts.n_sims) / (child.mcts.n_sims + 1)
        if child.mcts.n_sims > 0:
            value_score = -child.mcts.value
        else:
            value_score = 0
        value = prior_score + value_score
        return value

    @staticmethod
    def backpropagate(node: GameNode, value: float, to_play=0):
        while node is not None:
            # Scale the values down to probabilities in the range (0, 1) instead of (-1, 1)
            if node.mcts.value is None:
                node.mcts.value = 0

            node.mcts.value += value if node.to_play == to_play else -value
            node.mcts.n_sims += 1

            node = node.parent

    def get_value_and_expand(self, network, leaf: GameNode):
        leaf.expand()

        # Initialize policy vector
        arr = leaf.to_np(leaf.to_play)
        tensor = torch.Tensor(arr).unsqueeze(0)
        policy, value = network(tensor.to(self.device))
        policy = policy.to("cpu").squeeze()

        # Only valid moves should be non-zero
        valid_indices = torch.IntTensor([child.encode() for child in leaf.children])
        np_policy = np.zeros(policy.shape)
        np_policy[valid_indices] = torch.index_select(policy, 0, valid_indices).detach()

        leaf.mcts.policy = np_policy / np.sum(np_policy)

        # Set child prior
        for child in leaf.children:
            child.mcts.prior = leaf.mcts.policy[child.encode()].item()

        return value.item()

    @staticmethod
    def get_terminal_value(node: GameNode):
        state = node.finished()
        if state == GameState.UNDETERMINED:
            return None

        if state == GameState.WHITE_WON:
            value = 1.
        elif state == GameState.BLACK_WON:
            value = -1.
        else:
            value = 0

        if node.to_play == 0:
            return value
        else:
            return -value

    def process(self, game: Game, network):
        """
        Processes a node with a policy and returns the MCTS-augmented policy.
        :param p1:
        :param network:
        :param game:
        :return:
        """
        game.reset_children()
        self.get_value_and_expand(network, game.node)
        game.node.mcts.value = 0

        reached_terminal_state = False

        # Do N iterations of MCTS, building the tree based on the NN output.
        for i in range(self.iterations):
            leaf = self.select_best(game.node)

            value = self.get_terminal_value(leaf)
            if value is None:
                value = self.get_value_and_expand(network, leaf)
            # else:
            #     reached_terminal_state = True
            #
            # if reached_terminal_state:
            #     print("############### Reached terminal ################")
            #     print(game.node)
            #     print("\n")
            #     print(leaf)
            #     print("\n")
            #     print(self.create_policy_vector(game))
            #     input()

            self.backpropagate(leaf, value, to_play=leaf.to_play)
        # Create a new policy to fill with MCTS updated values
        updated_policy = self.create_policy_vector(game)
        return updated_policy

    def create_policy_vector(self, game):
        updated_policy = torch.zeros(game.node.mcts.policy.shape, device=self.device)
        for node in game.children():
            updated_policy[node.encode()] += node.mcts.n_sims
        updated_policy /= torch.sum(updated_policy)
        return updated_policy
