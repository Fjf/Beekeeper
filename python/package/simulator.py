from typing import Optional, Type, List

import pytorch_lightning
import torch

from MCTS import MCTS
from games.Game import Game


class Simulator:
    def __init__(self, game: Type[Game], mcts_iterations=100, device="cpu"):
        self.do_mcts = True
        self.game = game
        self.mcts_object = MCTS(mcts_iterations, device=device)
        self.temperature_threshold = 0.3

    def nn_move(self, network, game: Game, mcts=True):
        """
          Tile format (8 bits);
            [ NN C TTTTT ]

        Selects a move based on the policy vector and updates the board state.
        Returns the policy vector with which the move was chosen.

        :param network: the neural network for which to compute the move
        :param game: the gamestate for which to compute the move
        :param mcts: use mcts to compute an improvement policy?
        :return: the taken move index in the policy vector
        """

        # Select only best moves with low enough temperature
        # temperature = (float(game.turn_limit) - float(game.node.turn)) / float(game.turn_limit)
        temperature = game.node.turn < 15

        if mcts:
            policy = self.mcts_object.process(game, network, temperature=temperature)
        else:
            game.node.expand()
            policy, _ = network.predict(game.node.to_np(game.node.to_play))

            invalid_mask = torch.ones(policy.shape, dtype=torch.bool)
            for child in game.node.children:
                invalid_mask[child.encode()] = False

            policy[invalid_mask] = 0
            p_sum = torch.sum(policy)
            if p_sum == 0:
                policy = invalid_mask / torch.sum(invalid_mask)
            else:
                policy /= p_sum

        # Select a move based on updated_policy's weights out of the valid moves (encodings)
        best_idx = torch.multinomial(policy, 1).item()

        selected = None
        for child in game.node.children:
            if child.encode() == best_idx:
                selected = child
                break

        assert selected is not None, f"{best_idx} not " \
                                     f"in {[child.encode() for child in game.node.children]} " \
                                     f"(policy: {policy})" \
                                     f"mcts: {mcts}"
        game.select_child(selected)

        return policy

    def play(self,
             game: Game,
             networks: List[Optional[pytorch_lightning.LightningModule]],
             mcts=True):
        """
        Does a playout of the game with two neural networks, returns all board states and the final result.
        The returned zip contains all boards which were played by player 1 including the policy vector.
        This can later be used to train the neural network.

        :param mcts: Use MCTS for policy enhancement
        :param game: the board state from which to choose
        :param networks: the neural networks for all players
        :return: zip(boards, policies), result
        """
        policy_vectors = []
        # Play game until a terminal state has occurred.
        result = -1
        for i in range(game.turn_limit):

            nn = networks[game.to_play()]

            policy_vector = None
            if nn is None:
                game.ai_move(algorithm="random")
            else:
                policy_vector = self.nn_move(nn, game, mcts=mcts)
                policy_vector = policy_vector.to("cpu")

            policy_vectors.append(policy_vector)

            result = game.winner()

            if result is not None:
                break

        # Return all board states and the final result.
        return zip(game.history[:-1], policy_vectors), result

    def parallel_play(self, nn_perspective: int, net1: pytorch_lightning.LightningModule,
                      net2: Optional[pytorch_lightning.LightningModule] = None):
        """
        Wrapper for the play function preparing the data and initializing the Hive object.
        It will convert the arrays to tensors to be used by pytorch.
        This function is threadsafe.
        """
        tensors = []
        policy_vectors = []

        game = self.game()

        node_result, result = self.play(game, [net1, net2])
        node_result = list(node_result)
        for board_idx, (node, policy_vector) in enumerate(node_result):
            # Convert all data to be usable
            # if board_idx > len(node_result) - 5:
            #     print(node.to_np(node.to_play))
            #     print(policy_vector)
            if node.to_play == nn_perspective:
                tensors.append(torch.Tensor(node.to_np(node.to_play)))
                policy_vectors.append(policy_vector)

        # Convert game outcome to numerical value vector
        result_value = (
            0 if (result == -1)
            else 1 if (result == nn_perspective)
            else -1
        )
        result_values = [torch.Tensor([result_value])] * len(tensors)

        # Invert tensors
        # new_tensors = game.get_inverted(tensors)
        # if len(new_tensors) > 0:
        #     tensors += new_tensors
        #     policy_vectors += policy_vectors
        #     result_values += ([torch.Tensor([result_value * -1])] * len(new_tensors))

        return tensors, policy_vectors, result_values, nn_perspective
