import logging
import pickle

from mpi4py import MPI
import random
import socket
import time
from collections import defaultdict
from timeit import default_timer

import joblib as joblib
import torch
from joblib import delayed

from hive import Hive, HiveState, PlayerArguments
from hive_nn import HiveNN
from simulator import HiveSimulator
from train import School
import sys


def setup_logger():
    formatter = logging.Formatter("[%(asctime)s] [%(levelname)5s] --- %(message)s (%(filename)s:%(lineno)s)",
                                  "%Y-%m-%d %H:%M:%S")

    handler = logging.StreamHandler()
    handler.setFormatter(formatter)

    logger = logging.getLogger("Hive")
    logger.setLevel(logging.DEBUG)
    logger.addHandler(handler)
    return logger


def threading_test():
    def compute(i):
        time.sleep(i / 100)
        hive = Hive(track_history=True)
        while hive.finished() == HiveState.UNDETERMINED:
            hive.ai_move("random")

        print("Process", i)
        return hive.history, hive.history_idx

    start = default_timer()

    results = joblib.Parallel(n_jobs=6)(delayed(compute)(i) for i in range(20))
    for ix, i in enumerate(results):
        if not (i == results[0]).all():
            print(ix)

    print("Elapsed:", default_timer() - start)


def mcts_test():
    results = defaultdict(list)
    config = PlayerArguments()
    config.time_to_move = 1
    config.verbose = False
    config.mcts_constant = 0

    for i in range(100):
        hive = Hive()
        while hive.finished() == HiveState.UNDETERMINED:
            print("At turn", hive.turn())
            if hive.turn() % 2 == 0:
                hive.ai_move("mcts", config)
            else:
                hive.ai_move("random")
        print("Hoi")
        results[hive.finished()].append(hive.turn())

    print(results)


def winrate_test():
    def find_winning(h):
        for child in h.children():
            if child.finished() == HiveState.WHITE_WON:
                return child
        return None

    def random_no_lose(h):
        for _ in range(100):
            children = list(h.children())
            i = random.randint(0, len(children) - 1)
            if children[i].finished() != HiveState.BLACK_WON:
                return children[i]

        i = random.randint(0, len(h.children()))
        return children[i]

    results = defaultdict(int)
    for i in range(100):
        hive = Hive()
        while hive.finished() == HiveState.UNDETERMINED:
            if hive.turn() % 2 == 0:
                child = find_winning(hive)
                if not child:
                    child = random_no_lose(hive)

                hive.select_child(child)
            else:
                hive.ai_move("random")
        results[hive.finished()] += 1

    result_formatted = "\n\t".join(f"{k}: {v}" for k, v in results.items())
    print(result_formatted)


def main():
    # TODO: Read from commandline/config file
    n_sims = 500

    # Give workers their own logic
    if rank != MASTER_THREAD:
        return main_worker(n_sims=n_sims, mcts_iterations=100)

    logger = setup_logger()
    logger.debug("Python version")
    logger.debug(sys.version)

    if "login" in socket.gethostname():
        logger.error("Cannot execute on the login node.")
        exit(1)

    logger.info("Initializing school")
    school = School(simulations=n_sims)

    logger.info("Starting training")
    school.train(100, pretraining=False)


def main_worker(n_sims=100, mcts_iterations=100):
    while True:
        data = comm.bcast(None, root=MASTER_THREAD)
        networks = pickle.loads(data)
        simulator = HiveSimulator(mcts_iterations=mcts_iterations)

        for i in range(rank - 1, n_sims, comm.Get_size() - 1):
            data = simulator.parallel_play(i, *networks)
            comm.send(data, MASTER_THREAD)


if __name__ == "__main__":
    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    MASTER_THREAD = 0
    assert(comm.Get_size() > 1)
    main()
