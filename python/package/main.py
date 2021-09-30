import logging
import multiprocessing
import random
import socket
import time
from collections import defaultdict
from timeit import default_timer

import joblib as joblib
from joblib import delayed

from hive import Hive, HiveState
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
    logger = setup_logger()
    logger.debug("Python version")
    logger.debug(sys.version)

    if "login" in socket.gethostname():
        logger.error("Cannot execute on the login node.")
        exit(1)

    logger.info("Initializing school")
    school = School()

    logger.info("Starting training")
    school.train(100, simulations=100)


if __name__ == "__main__":
    main()
