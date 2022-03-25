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
from typing import Tuple, Optional

import math
import numpy as np
import torch

from games.Game import Game, GameNode
from games.hive.hive import Node, lib, PlayerArguments, MCTSData
from games.utils import GameState


class MCTS:
    def __init__(self, iterations=100, device="cuda"):
        self.iterations = iterations
        self.mcts_constant = 1
        self.logger = logging.getLogger("Hive")
        self.device = torch.device(device)

    def select_best(self, root: GameNode, network) -> Tuple[GameNode, Optional[int]]:
        parent = root

        while True:
            # Iterate all children of this parent, to find the best child to explore.
            children = parent.get_children()

            if len(children) == 0:
                # Parent has no more children (terminal condition)
                return parent, None

            # First time reaching this node.
            if parent.mcts.policy is None:
                # Initialize policy vector
                arr = parent.to_np(parent.turn() % 2)
                tensor = torch.Tensor(arr.reshape(1, *arr.shape))
                policy, nn_value = network(tensor.to(self.device))

                parent.mcts.policy = policy[0].to("cpu")
                value = nn_value.item()

                del nn_value
                del tensor
                del policy
                # Free up some cuda memory.
                # torch.cuda.empty_cache()

                # Initialize children.
                for child in children:
                    # Initialize child value to the inverted value of this, best value for p1 is worst for p2
                    child.mcts.value = (1 - parent.mcts.policy[child.encode()]).item()
                    child.mcts.n_sims = 1
                return parent, value

            ################################
            # Select best child to explore.
            ################################
            best_value, best = 0, None
            for child in children:
                value = child.mcts.value
                n_sims = child.mcts.n_sims
                parent_sims = parent.mcts.n_sims

                value = value / n_sims + self.mcts_constant * math.sqrt(math.log(parent_sims) / n_sims)
                if best_value < value:
                    best_value, best = value, child

            assert (best is not None)
            parent = best

    @staticmethod
    def cascade_value(leaf: GameNode, value: float, multiplier=1):
        while leaf is not None:
            # Scale the values down to probabilities in the range (0, 1) instead of (-1, 1)
            leaf.mcts.value += (value + 1.) / 2 * multiplier
            leaf.mcts.n_sims += 1 * multiplier

            leaf = leaf.parent
            # Invert value every step (good move for p1 is bad for p2)
            value = - value

    def process(self, root: Game, network):
        """
        Processes a node with a policy and returns the MCTS-augmented policy.
        :param p1:
        :param network:
        :param root:
        :return:
        """
        if network.device != self.device:
            network = network.to(self.device)

        # Do N iterations of MCTS, building the tree based on the NN output.
        for i in range(self.iterations):
            leaf, value = self.select_best(root.node, network)
            assert root.node.mcts.policy.device == torch.device("cpu"), f"Device: '{root.node.mcts.policy.device}'"
            state = leaf.finished()
            if state != GameState.UNDETERMINED:
                # If it is a terminal state, force update the policy vector.
                if state == GameState.WHITE_WON:
                    value = 1.
                elif state == GameState.BLACK_WON:
                    value = -1.
                else:
                    value = 0

                # Invert leaf value if not player 0
                if leaf.turn() % 2 == 0:
                    value = -value

            self.cascade_value(leaf, value)

        # Create a new policy to fill with MCTS updated values
        updated_policy = torch.zeros(root.node.mcts.policy.shape)
        updated_policy += 0.000001

        for node in root.children():
            games = node.mcts.n_sims
            updated_policy[node.encode()] += (games - 1)

        return updated_policy / torch.sum(updated_policy)
