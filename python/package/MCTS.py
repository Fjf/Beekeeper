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
import math
from typing import Tuple, Optional, List

import numpy as np
import torch

from games.Game import Game, GameNode
from games.utils import GameState


class MCTS:
    def __init__(self, iterations=100, device="cpu", mcts_batch_size=16):
        self.iterations = iterations
        self.mcts_constant = 1
        self.logger = logging.getLogger("Hive")
        self.device = torch.device(device)
        self.batch_size = mcts_batch_size

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

                value = child.mcts.value / child.mcts.n_sims + self.mcts_constant * math.sqrt(
                    math.log(parent.mcts.n_sims) / child.mcts.n_sims)
                if best_value < value:
                    best_value, best = child.mcts.value, child

            parent = best

    def blatyp(self, nodes, n_nodes, policies, nn_values, result):
        for i, node in enumerate(nodes[:n_nodes]):
            policy, nn_value = policies[i].unsqueeze(dim=0), nn_values[i].unsqueeze(dim=0)
            # policy, nn_value = network(tensor.to(self.device))
            node.mcts.policy = policy[0]
            value = nn_value.item()
            result.append((node, value))

            # Initialize children.
            for child in node.get_children():
                # Initialize child value to the inverted value of this, best value for p1 is worst for p2
                child.mcts.value = (1 - node.mcts.policy[child.encode()]).item()
                child.mcts.n_sims = 1

    def process_vector(self, nodes, n_nodes, network):
        result = []

        numpy_boards = [node.to_np(node.turn() % 2) for node in nodes[:n_nodes]]

        input_tensor = (torch
                        .from_numpy(np.stack(numpy_boards))
                        .to(self.device, dtype=torch.float32, non_blocking=True))
        policies, nn_values = network(input_tensor)

        nn_values, policies = (
            nn_values.to("cpu", non_blocking=True),
            policies.to("cpu", non_blocking=True)
        )

        self.blatyp(nodes, n_nodes, policies, nn_values, result)
        return result

    def vectorized_select_best(self, root: GameNode, network) -> List[Tuple[GameNode, Optional[int]]]:
        global n_nodes_added
        n_nodes_added = 0
        nodes = np.empty(self.batch_size, dtype=object)
        skip_me = set()
        result = []

        def vectorized_add_check(p):
            global n_nodes_added
            nodes[n_nodes_added] = p
            n_nodes_added += 1
            return not n_nodes_added == self.batch_size

        parent = root
        while parent is not None:
            # Iterate all children of this parent, to find the best child to explore.
            children = parent.get_children()

            if len(children) == 0:
                skip_me.add(parent)
                result.append((parent, None))
                parent = parent.parent
                continue

            # First time reaching this node, or it has no children.
            if parent.mcts.policy is None:
                if not vectorized_add_check(parent):
                    break
                skip_me.add(parent)
                parent = parent.parent
                continue

            ################################
            # Select best child to explore.
            ################################
            best_value, best = 0, None
            for child in children:
                if child in skip_me:
                    continue
                # Only explore a leaf once per cycle
                child.mcts.value = (
                        child.mcts.value / child.mcts.n_sims
                        + self.mcts_constant * math.sqrt(math.log(parent.mcts.n_sims) / child.mcts.n_sims)
                )
                if best_value < child.mcts.value:
                    best_value, best = child.mcts.value, child

            if best is None:
                skip_me.add(parent)
                parent = parent.parent
            else:
                parent = best

        if n_nodes_added > 0:
            result += self.process_vector(nodes, n_nodes_added, network)
        return result

    @staticmethod
    def cascade_value(leaf: GameNode, value: float):
        while leaf is not None:
            # Scale the values down to probabilities in the range (0, 1) instead of (-1, 1)
            leaf.mcts.value += (value + 1.) / 2
            leaf.mcts.n_sims += 1

            leaf = leaf.parent
            # Invert value every step (good move for p1 is bad for p2)
            value = - value

    @staticmethod
    def side_dependent_value(leaf, value):
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

        return value

    def process(self, root: Game, network):
        """
        Processes a node with a policy and returns the MCTS-augmented policy.
        :param p1:
        :param network:
        :param root:
        :return:
        """

        # Do N iterations of MCTS, building the tree based on the NN output.
        i = 0
        while i < self.iterations:
            result = self.select_best(root.node, network)

            for leaf, value in [result]:
                value = self.side_dependent_value(leaf, value)
                self.cascade_value(leaf, value)
            i += len(result)

        # Create a new policy to fill with MCTS updated values
        updated_policy = torch.zeros(root.node.mcts.policy.shape) + 0.000001

        for node in root.children():
            games = node.mcts.n_sims
            updated_policy[node.encode()] += (games - 1)

        return updated_policy / torch.sum(updated_policy)
