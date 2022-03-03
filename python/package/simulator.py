import logging
import random
from typing import Optional

import pytorch_lightning
import torch

from MCTS import MCTS
from games.Game import Game
from games.utils import GameState, Perspectives


class Simulator:
    def __init__(self, game, mcts_iterations=100):
        self.do_mcts = True
        self.game = game
        self.mcts_object = MCTS(mcts_iterations)
        self.temperature_threshold = 0.5

    @staticmethod
    def select_move(policy, filter_idx=None, best=False):
        """
        Select a move based on the provided policy vector only selecting moves according to the provided filter vector.
        The return value is  0 <= return < len(filter_idx)

        :param best:
        :param policy: a vector with move probabilities
        :param filter_idx: a vector defining the moves from which to choose
        :return: the index of the chosen move in the filter vector
        """
        if sum(policy[filter_idx] == 0):  # Select random move if policy sums to 0
            logging.getLogger("hive") \
                .warning("Filtered policy summed to 0, selecting random move. Training is probably going wrong.")
            return random.choice(range(len(filter_idx)))

        # Pick the best move or a random one based on policy probabilities
        if best:
            return torch.argmax(policy[filter_idx].flatten())
        else:
            probabilities = policy[filter_idx].flatten()
            return torch.multinomial(probabilities, 1).item()

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
        perspective = game.to_move()

        if mcts:
            policy = self.mcts_object.process(game, network)
        else:
            # Fetch array from internal memory and remove the tile number.
            arr = game.node.to_np(perspective)
            arr = arr.reshape(1, *arr.shape)

            # Convert data to tensorflow usable format
            data = torch.Tensor(arr)

            # Get policy vector
            policy, _ = network(data)
            policy = policy[0]

        # Convert children to usable format.
        children = game.node.get_children()
        encodings = [child.encode() for child in children]

        # Select only best moves after high enough temperature
        temperature = float(game.node.turn()) / float(game.turn_limit)

        # Select a move based on updated_policy's weights out of the valid moves (encodings)
        best_idx = self.select_move(policy, filter_idx=encodings, best=temperature > self.temperature_threshold)

        game.select_child(children[best_idx])
        return policy

    def play(self, game: Game, p1: Optional[pytorch_lightning.LightningModule],
             p2: Optional[pytorch_lightning.LightningModule], mcts=True):
        """
        Does a playout of the game with two neural networks, returns all board states and the final result.
        The returned zip contains all boards which were played by player 1 including the policy vector.
        This can later be used to train the neural network.

        :param mcts: Use MCTS for policy enhancement
        :param game: the board state from which to choose
        :param p1: the neural network for player 1
        :param p2: the neural network for player 2, if None, use random agent instead
        :return: zip(boards, policies), result
        """
        policy_vectors = []
        # Play game until a terminal state has occurred.
        result = GameState.UNDETERMINED
        for i in range(game.turn_limit):

            nn = p1 if game.to_move() == Perspectives.PLAYER1 else p2

            policy_vector = None
            if nn is None:
                game.ai_move(algorithm="random")
            else:
                policy_vector = self.nn_move(nn, game, mcts=mcts)

            policy_vectors.append(policy_vector)

            result = game.finished()

            if result != GameState.UNDETERMINED:
                break

        # Return all board states and the final result.
        return zip(game.history[:-1], policy_vectors), result

    def parallel_play(self, nn_perspective: Perspectives, net1: pytorch_lightning.LightningModule,
                      net2: Optional[pytorch_lightning.LightningModule] = None):
        """
        Wrapper for the play function preparing the data and initializing the Hive object.
        It will convert the arrays to tensors to be used by pytorch.
        This function is threadsafe.
        """
        tensors = []
        policy_vectors = []

        game = self.game()

        node_result, result = self.play(game, net1, net2)

        for board_idx, (node, policy_vector) in enumerate(node_result):
            if board_idx % 2 == 0:
                perspective = Perspectives.PLAYER1
            else:
                perspective = Perspectives.PLAYER2

            # Convert all data to be usable
            if perspective == nn_perspective:
                arr = node.to_np(nn_perspective)
                tensors.append(torch.Tensor(arr))
                policy_vectors.append(policy_vector)

        # Convert game outcome to numerical value.
        result_value = 0
        if (result == GameState.WHITE_WON and nn_perspective == Perspectives.PLAYER1) \
                or (result == GameState.BLACK_WON and nn_perspective == Perspectives.PLAYER2):
            result_value = 1
        if (result == GameState.BLACK_WON and nn_perspective == Perspectives.PLAYER1) \
                or (result == GameState.WHITE_WON and nn_perspective == Perspectives.PLAYER2):
            result_value = -1

        # TODO: Wrap this in a namedtuple
        return tensors, policy_vectors, result_value, nn_perspective
