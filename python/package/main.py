import argparse
import logging
import pickle
import socket
import sys
import traceback
from typing import Type

import pytorch_lightning
import torch.cuda
from mpi4py import MPI
from pytorch_lightning import Trainer
from torch.utils.tensorboard import SummaryWriter

from games.Game import Game
from games.utils import Perspectives
from model_performance import performance
from mpi_packet import MPIPacket, MPIPacketState
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


def main():
    logger = setup_logger()
    parser = setup_parser()
    args = parser.parse_args()

    if "login" in socket.gethostname():
        logger.error("Cannot execute on the login node.")
        exit(1)

    n_model_updates = args.n_model_updates
    model_dir = args.model_dir

    # Fetch game and corresponding neural network from subclasses automatically.
    game_type = [s for s in Game.__subclasses__()
                 if s.__name__ == args.game][0]
    network_type = [s for s in pytorch_lightning.LightningModule.__subclasses__()
                    if args.game.lower() in s.__name__.lower()][0]
    del args.game

    # Optimize backends
    torch.backends.cudnn.benchmark = True
    torch.set_float32_matmul_precision('medium')

    # Give workers their own logic
    if rank != MASTER_THREAD:
        assert not args.test_performance
        return main_worker(game_type, network_type, **vars(args))

    # Setup tensorboard on Master process
    writer = SummaryWriter('runs/hive_experiment_1')

    logger.debug("Python version")
    logger.debug(sys.version)

    if args.test_performance:
        performance(model_dir, game_type, network_type)
    else:
        logger.info(f"Initializing school for {game_type.__name__} with PL: {network_type.__name__}")
        school = School(game_type,
                        network_type,
                        **vars(args))

        logger.info("Starting training")
        # import pdb
        # pdb.set_trace()
        school.train(n_model_updates,
                     pretraining=False,
                     stored_model_filename=args.model,
                     **vars(args))

        # Done processing, kill workers:
        packet = MPIPacket(MPIPacketState.TERMINATE, None)
        school.comm.bcast(pickle.dumps(packet))


def main_worker(game, model_type: Type[pytorch_lightning.LightningModule], n_sims=100, mcts_iterations=100,
                mcts_batch_size=16, device="cuda", **kwargs):
    import time
    logger = logging.getLogger("Child")

    n_workers = comm.Get_size() - 1

    # Initialize models, and set them to eval mode.
    network1 = model_type()
    network2 = model_type()

    # If running on cuda, select a specific available device based on rank.
    if device == "cuda":
        gpu_idx = rank % torch.cuda.device_count()
        device = f"{device}:{gpu_idx}"

    # Initialize MCTS simulator and move networks to correct device.
    simulator = Simulator(game, mcts_iterations=mcts_iterations, device=device)
    network1 = network1.to(device)
    network1.eval()
    network2 = network2.to(device)
    network2.eval()

    logger.info(f"Process {rank} running on device {simulator.mcts_object.device} ({MPI.Get_processor_name()})")

    # Jit script compiles the models for better performance.

    # compiled_network1 = network1
    # compiled_network2 = network2

    with torch.no_grad():
        while True:
            # Load and unpickle data
            raw_packet: MPIPacket = comm.bcast(None, root=MASTER_THREAD)
            packet = pickle.loads(raw_packet)
            if packet.state == MPIPacketState.TERMINATE:
                break

            for i in range(rank - 1, n_sims, n_workers):
                state_dict1, state_dict2 = packet.data

                if i % 2 == 0:
                    perspective = 0
                else:
                    perspective = 1

                # Swap neural networks to let training network experience both perspectives.
                if perspective == 1:
                    state_dict1, state_dict2 = state_dict2, state_dict1

                network1.load_state_dict(state_dict1)
                network2.load_state_dict(state_dict2)
                compiled_network1 = network1.to_torchscript()
                compiled_network2 = network2.to_torchscript()

                # Jit script compiles the models for better performance.
                # optimized_network1 = torch.jit.optimize_for_inference(compiled_network1)
                # optimized_network2 = torch.jit.optimize_for_inference(compiled_network2)
                data = simulator.parallel_play(perspective, compiled_network1, compiled_network2)
                comm.send((i, *data), MASTER_THREAD)

            # We don't need to be in a busy-loop.
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
        MPI.COMM_WORLD.Abort(1)
