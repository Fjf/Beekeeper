import argparse
import logging
import pickle
import socket

import pytorch_lightning
import sys
import traceback
from mpi4py import MPI
from torch.utils.tensorboard import SummaryWriter

from games.Game import Game
from simulator import Simulator
from train import School, Perspectives


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

    # Fetch game and corresponding neural network from subclasses automatically.
    game = [s for s in Game.__subclasses__()
            if s.__name__ == args.game][0]
    network = [s for s in pytorch_lightning.LightningModule.__subclasses__()
               if args.game.lower() in s.__name__.lower()][0]

    # Give workers their own logic
    if rank != MASTER_THREAD:
        return main_worker(game, n_sims=n_sims, mcts_iterations=mcts_iterations)

    # Setup tensorboard on Master process
    writer = SummaryWriter('runs/hive_experiment_1')

    logger.debug("Python version")
    logger.debug(sys.version)

    logger.info(f"Initializing school for {game.__name__} with PL: {network.__name__}")
    school = School(game, network, simulations=n_sims, n_old_data=args.n_data_reuse)

    logger.info("Starting training")
    school.train(n_model_updates, pretraining=False)


def main_worker(game, n_sims=100, mcts_iterations=100):
    while True:
        data = comm.bcast(None, root=MASTER_THREAD)
        networks = pickle.loads(data)
        simulator = Simulator(game, mcts_iterations=mcts_iterations)

        for i in range(rank - 1, n_sims, comm.Get_size() - 1):
            network1, network2 = networks

            if i % 2 == 1:
                perspective = Perspectives.PLAYER1
            else:
                perspective = Perspectives.PLAYER2

            # Swap neural networks to let training network experience both perspectives.
            if perspective == Perspectives.PLAYER2:
                network1, network2 = network2, network1

            data = simulator.parallel_play(perspective, network1, network2)
            comm.send((i, *data), MASTER_THREAD)


if __name__ == "__main__":
    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    MASTER_THREAD = 0
    assert (comm.Get_size() > 1)
    try:
        main()
    except Exception as e:  # noqa
        traceback.print_exc()
