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


from ctypes import POINTER

import numpy as np
import torch

from hive import Node
from utils import remove_tile_idx, HiveState




class MCTSNode:
    def __init__(self, node: POINTER(Node)):
        self.node = node


class HiveMCTS:
    def __init__(self, iterations=100):
        self.iterations = iterations

    def select_best(self, node):
        # TODO: Implement for best node selection using UCT.
        ...

    @staticmethod
    def process(node: POINTER(Node), policy, network, p1=True):
        """
        Processes a node with a policy and returns the MCTS-augmented policy.
        :param p1:
        :param network:
        :param node:
        :param policy:
        :return:
        """
        # TODO: Use the amount of iterations to build a tree for MCTS + updating policy vectors.

        children = node.contents.get_children()
        updated_policy = torch.zeros(len(policy))

        best = -1
        for i, child in enumerate(list(children)):
            idx = child.encode()

            # Fetch array from internal memory and remove the tile number.
            arr = child.to_np()
            arr = remove_tile_idx(arr)
            arr = arr.reshape(1, 5, 26, 26)

            # Convert data to tensorflow usable format
            data = torch.Tensor(arr)

            state = child.finished()
            if state == HiveState.UNDETERMINED:
                # Get tensor of preferred outputs by priority.
                _, value = network(data)
                updated_policy[idx] = value
            else:
                # If it is a terminal state, force update the policy vector.
                if state == HiveState.WHITE_WON:
                    value = 1
                elif state == HiveState.BLACK_WON:
                    value = 0
                else:
                    value = 0.5
                updated_policy[idx] = value

            if value > best:
                best = value

        updated_policy[-1] = best
        return updated_policy
