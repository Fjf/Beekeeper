import argparse
import logging

from games.Game import Game


def setup_logger():
    formatter = logging.Formatter("[%(asctime)s] [%(levelname)5s] --- %(message)s (%(filename)s:%(lineno)s)",
                                  "%Y-%m-%d %H:%M:%S")

    handler = logging.StreamHandler()
    handler.setFormatter(formatter)

    logger = logging.getLogger("Hive")
    logger.setLevel(logging.DEBUG)
    logger.addHandler(handler)
    logger = logging.getLogger("Child")
    logger.setLevel(logging.DEBUG)
    logger.addHandler(handler)
    return logger


def setup_parser():
    parser = argparse.ArgumentParser(description="Parallelized AlphaZero implementation of Hive.")
    parser.add_argument("--n_sims", "-n", type=int, default=200,
                        help="The amount of game-simulations to do for each data step.")
    parser.add_argument("--mcts_batch_size", type=int, default=16,
                        help="The amount of MCTS simulation boards to aggregate into a batch before passing it through "
                             "the network. Might result in less accurate results, due to different tree searching.")
    parser.add_argument("--batch_size", type=int, default=256,
                        help="Training batch_size.")
    # parser.add_argument("--learning_rate", type=int, default=0.01,
    #                     help="Learning rate for Adam.")
    parser.add_argument("--mcts_iterations", "-m", type=int, default=400,
                        help="The amount of MCTS playouts to do per move.")
    parser.add_argument("--n_model_updates", "-u", type=int, default=100,
                        help="The amount of model updates to do during training before terminating.")
    parser.add_argument("--n_data_reuse", "-d", type=int, default=1,
                        help="The amount of data generation iterations being used for a single training update.")
    subclasses = [s.__name__ for s in Game.__subclasses__()]
    parser.add_argument("--game", "-g", type=str, choices=subclasses, default=subclasses[0],
                        help="The current game to play.")
    parser.add_argument("--model", "-M", type=str, default=None,
                        help="The model on disk to continue from.")
    parser.add_argument("--model_dir", type=str, default="model",
                        help="The folder to store the models in.")
    parser.add_argument("--data_dir", type=str, default=None,
                        help="The folder to store all generated data in, if none provided, dont store at all.")
    parser.add_argument("--test_performance", type=bool, default=False,
                        help="Pit all generated models against each other and save a winrate matrix.")
    parser.add_argument("--device", type=str, default="cuda",
                        help="Device to run on, when specifying cuda, all workers will be split across the available "
                             "cuda devices evenly.")

    return parser
