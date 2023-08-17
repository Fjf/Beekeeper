import logging
import pickle
import sys
import traceback
from typing import Type

import pytorch_lightning
import torch.cuda
from mpi4py import MPI
from torch.utils.tensorboard import SummaryWriter

from games.Game import Game
from model_performance import performance
from mpi_packet import MPIPacket, MPIPacketState
from simulator import Simulator
from train import School
from utils import setup_logger, setup_parser


def main():
    logger = setup_logger()
    parser = setup_parser()
    args = parser.parse_args()

    # Fetch game and corresponding neural network from subclasses automatically.
    game_type = [s for s in Game.__subclasses__()
                 if s.__name__ == args.game][0]
    network_type = [s for s in pytorch_lightning.LightningModule.__subclasses__()
                    if args.game.lower() in s.__name__.lower()][0]
    del args.game

    # Optimize backends
    torch.backends.cudnn.benchmark = True
    # torch.set_float32_matmul_precision('medium')

    # Give workers their own logic
    if rank != MASTER_THREAD:
        assert not args.test_performance
        return main_worker(game_type, network_type, **vars(args))

    # Setup tensorboard on Master process
    writer = SummaryWriter('runs/hive_experiment_1')

    logger.debug("Python version")
    logger.debug(sys.version)

    if args.test_performance:
        performance(args.model_dir, game_type, network_type)
    else:
        logger.info(f"Initializing school for {game_type.__name__} with PL: {network_type.__name__}")
        school = School(game_type,
                        network_type,
                        comm=MPI.COMM_WORLD,
                        **vars(args))

        logger.info("Starting training")
        # import pdb
        # pdb.set_trace()
        school.train(args.n_model_updates,
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

                # Jit script compiles the models for better performance.
                # optimized_network1 = torch.jit.optimize_for_inference(compiled_network1)
                # optimized_network2 = torch.jit.optimize_for_inference(compiled_network2)
                data = simulator.parallel_play(perspective, network1, network2)
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
