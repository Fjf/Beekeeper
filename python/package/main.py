import argparse
import logging
import pickle
import socket
from typing import Type

import pytorch_lightning
import sys
import traceback

import torch.cuda
from mpi4py import MPI
from torch.utils.tensorboard import SummaryWriter

from games.Game import Game
from games.utils import Perspectives
from simulator import Simulator
from train import School


def setup_logger():
    formatter = logging.Formatter("[%(asctime)s] [%(levelname)5s] --- %(message)s (%(filename)s:%(lineno)s)",
                                  "%Y-%m-%d %H:%M:%S")

    handler = logging.StreamHandler()
    handler.setFormatter(formatter)

    logger = logging.getLogger("Hive")
    logger.setLevel(logging.DEBUG)
    logger.addHandler(handler)
    return logger


def setup_parser():
    parser = argparse.ArgumentParser(description="Parallelized AlphaZero implementation of Hive.")
    parser.add_argument("--n_sims", "-n", type=int, default=200,
                        help="The amount of game-simulations to do for each data step.")
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
    return parser


def main():
    logger = setup_logger()
    parser = setup_parser()
    args = parser.parse_args()

    if "login" in socket.gethostname():
        logger.error("Cannot execute on the login node.")
        exit(1)

    n_sims = args.n_sims
    mcts_iterations = args.mcts_iterations
    n_model_updates = args.n_model_updates
    model_dir = args.model_dir
    data_dir = args.data_dir

    # Fetch game and corresponding neural network from subclasses automatically.
    game_type = [s for s in Game.__subclasses__()
                 if s.__name__ == args.game][0]
    network_type = [s for s in pytorch_lightning.LightningModule.__subclasses__()
                    if args.game.lower() in s.__name__.lower()][0]

    # Give workers their own logic
    if rank != MASTER_THREAD:
        return main_worker(game_type, network_type, n_sims=n_sims, mcts_iterations=mcts_iterations)

    # Setup tensorboard on Master process
    writer = SummaryWriter('runs/hive_experiment_1')

    logger.debug("Python version")
    logger.debug(sys.version)

    logger.info(f"Initializing school for {game_type.__name__} with PL: {network_type.__name__}")
    school = School(game_type, network_type, simulations=n_sims, n_old_data=args.n_data_reuse, model_dir=model_dir,
                    data_dir=data_dir)

    logger.info("Starting training")
    school.train(n_model_updates, pretraining=False, stored_model_filename=args.model)


def main_worker(game, model_type: Type[pytorch_lightning.LightningModule], n_sims=100, mcts_iterations=100):
    import time

    network1 = model_type()
    network2 = model_type()

    while True:
        data = comm.bcast(None, root=MASTER_THREAD)
        state_dicts = pickle.loads(data)
        simulator = Simulator(game, mcts_iterations=mcts_iterations)

        for i in range(rank - 1, n_sims, comm.Get_size() - 1):
            state_dict1, state_dict2 = state_dicts

            if i % 2 == 0:
                perspective = Perspectives.PLAYER1
            else:
                perspective = Perspectives.PLAYER2

            # Swap neural networks to let training network experience both perspectives.
            if perspective == Perspectives.PLAYER2:
                state_dict1, state_dict2 = state_dict2, state_dict1

            network1.load_state_dict(state_dict1)
            network2.load_state_dict(state_dict2)

            data = simulator.parallel_play(perspective, network1, network2)
            comm.send((i, *data), MASTER_THREAD)

            del data
            torch.cuda.empty_cache()

        # We don't need to be in a busyloop.
        time.sleep(0.2)


if __name__ == "__main__":
    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    MASTER_THREAD = 0
    assert (comm.Get_size() > 0)
    try:
        main()
    except Exception as e:  # noqa
        traceback.print_exc()
