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
import logging
from ctypes import POINTER, pointer

import math
import numpy as np
import torch

from games.Game import Game, GameNode
from games.hive.hive import Node, lib, PlayerArguments, MCTSData
from games.utils import GameState


class MCTS:
    def __init__(self, iterations=100):
        self.iterations = iterations
        self.mcts_constant = 1
        self.logger = logging.getLogger("Hive")

    @staticmethod
    def prepare_mcts(root: GameNode):
        root.mcts.value = 0
        root.mcts.n_sims = 0

    def select_best(self, root: GameNode, network) -> GameNode:
        parent = root

        while True:
            parent_sims = parent.mcts.n_sims

            best_value, best = 0, None
            # Iterate all children of this parent, to find the best child to explore.
            children = parent.get_children()

            if len(children) == 0:
                # Parent has no more children (terminal condition)
                return parent

            # First time reaching this node.
            if parent.mcts.policy is None:
                # Initialize policy vector
                arr = parent.to_np(parent.turn() % 2)
                tensor = torch.Tensor(arr.reshape(1, *arr.shape))

                policy, value = network(tensor)
                policy = policy[0]  # Extract first from batch
                parent.mcts.policy = policy

                # Initialize children.
                for child in children:
                    child.mcts.value = policy[child.encode()]
                    child.mcts.n_sims = 1
                return parent

            for child in children:
                value = child.mcts.value
                n_sims = child.mcts.n_sims

                value = value / n_sims + self.mcts_constant * math.sqrt(math.log(parent_sims) / n_sims)
                if best_value < value:
                    best_value, best = value, child

            assert (best is not None)
            parent = best

    @staticmethod
    def cascade_value(leaf: GameNode, value: int, multiplier=1):
        while leaf is not None:
            # Scale the values down to probabilities in the range (0, 1) instead of (-1, 1)
            leaf.mcts.value += (value + 1) / 2 * multiplier
            leaf.mcts.n_sims += 1 * multiplier

            leaf = leaf.parent
            # Invert value every step (good move for p1 is bad for p2)
            value = - value

    def process(self, root: Game, policy, network):
        """
        Processes a node with a policy and returns the MCTS-augmented policy.
        :param p1:
        :param network:
        :param root:
        :param policy:
        :return:
        """

        # Do N iterations of MCTS, building the tree based on the NN output.
        for i in range(self.iterations):
            leaf = self.select_best(root.node, network)
            state = leaf.finished()

            if state == GameState.UNDETERMINED:
                # Fetch array from internal memory
                arr = leaf.to_np(leaf.turn() % 2)
                tensor = torch.Tensor(arr.reshape(1, *arr.shape))

                # Get predicted game value from network
                _, value = network(tensor)
                value = value.item()  # Tensor should have one value so extract the value
            else:
                # If it is a terminal state, force update the policy vector.
                if state == GameState.WHITE_WON:
                    value = 1.
                elif state == GameState.BLACK_WON:
                    value = -1
                else:
                    value = 0

            # Invert leaf value
            if leaf.turn() % 2 == 0:
                value = -value

            self.cascade_value(leaf, value)

        # Create a new policy to fill with MCTS updated values
        updated_policy = torch.zeros(policy.shape)
        updated_policy += 0.000001

        tot_sims = sum(node.mcts.n_sims for node in root.children())
        for node in root.children():
            games = node.mcts.n_sims

            assert (games != 0)

            updated_policy[node.encode()] = games / tot_sims

        updated_policy = torch.pow(updated_policy, 2)
        return updated_policy / torch.sum(updated_policy)
