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

from ctypes import POINTER, pointer

import numpy as np
import torch

from hive import Node, lib, PlayerArguments, MCTSData
from utils import remove_tile_idx, HiveState


class HiveMCTS:
    def __init__(self, iterations=100):
        self.iterations = iterations
        self.pargs = PlayerArguments()
        self.pargs.mcts_constant = 2

    def select_best(self, root: POINTER(Node)):
        return lib.mcts_select_leaf(root, pointer(self.pargs))

    def process(self, root: POINTER(Node), policy, network):
        """
        Processes a node with a policy and returns the MCTS-augmented policy.
        :param p1:
        :param network:
        :param root:
        :param policy:
        :return:
        """

        # Prepare MCTS and keep all children in memory always.
        lib.mcts_prepare(root, pointer(self.pargs))
        root.contents.data.contents.keep = True
        root.contents.generate_moves()

        # Do N iterations of MCTS, building the tree based on the NN output.
        for i in range(self.iterations):
            leaf = self.select_best(root)

            state = leaf.contents.finished()

            if state == HiveState.UNDETERMINED:
                # Add the children of this leaf to the pool for next MCTS iteration to search through
                leaf.contents.generate_moves()
                # Tell MCTS to not free this node after passing it
                leaf.contents.data.contents.keep = True

                # Fetch array from internal memory and remove the tile number.
                arr = remove_tile_idx(leaf.contents.to_np()).reshape(1, 5, 26, 26)
                # Convert data to tensorflow usable format
                tensor = torch.Tensor(arr)

                # Get predicted game value from network
                _, value = network(tensor)
                value = value[0]  # Batch size is 1 so extract the value

                # Round neural net value to a win/loss
                value = HiveState.WHITE_WON.value if value > 0.5 else HiveState.BLACK_WON.value
            else:
                # If it is a terminal state, force update the policy vector.
                value = state.value

            lib.mcts_cascade_result(root, leaf, value)

        # Create a new policy to fill with MCTS updated values
        updated_policy = torch.zeros(len(policy))

        for node in root.contents.get_children():
            data: POINTER(MCTSData) = node.data
            games = data.contents.white + data.contents.black
            updated_policy[node.encode()] = (data.contents.white / games) if games != 0 else 0.5

        return updated_policy
