import os
import pickle

import itertools
import subprocess
from _collections import defaultdict

N_ITER = 5


def dd():
    return 1500


data = defaultdict(dd)


class Configuration(object):
    def __init__(self, alg="random", c=1, p=False, t=1., f=False, e=0):
        self.algorithm = alg
        self.mcts_constant = str(c)
        self.mcts_prioritization = p
        self.time = str(t)
        self.first_play_urgency = f
        self.evaluation_function = str(e)

    def to_cmd(self, player1=True):
        if player1:
            prio = "-P" if self.mcts_prioritization is True else ""
            fpu = "-F" if self.first_play_urgency is True else ""
            return ["-A", self.algorithm, prio, fpu, "-C", self.mcts_constant, "-T", self.time, "-E", self.evaluation_function]
        else:
            prio = "-p" if self.mcts_prioritization is True else ""
            fpu = "-f" if self.first_play_urgency is True else ""
            return ["-a", self.algorithm, prio, fpu, "-c", self.mcts_constant, "-t", self.time, "-e", self.evaluation_function]

    def __repr__(self):
        return "%s|%s|%d|%s|%d|%s" % (
            self.algorithm, self.mcts_constant, self.mcts_prioritization, self.time, self.first_play_urgency, self.evaluation_function)


def expect_result(p1, p2):
    exp = (p2 - p1) / 400.0
    return 1 / ((10.0 ** exp) + 1)


def award_elo(winner: Configuration, loser: Configuration, definitive=1.):
    win_elo = data[str(winner)]
    lose_elo = data[str(loser)]

    result = expect_result(win_elo, lose_elo)

    data[str(winner)] += 20 * (definitive - result)
    data[str(loser)] -= 20 * (definitive - result)


def playout(p1: Configuration, p2: Configuration):
    base = ["../c/cmake-build-debug/hive_run"]
    command = base + p1.to_cmd() + p2.to_cmd(False)

    print(" ".join(command))

    result = subprocess.check_output(command).decode("utf-8")
    turn_limit = True
    for line in result.split("\n"):
        if "won" in line:
            print(line)
            turn_limit = False
            if "1" in line:
                award_elo(p1, p2)
            elif "2" in line:
                award_elo(p2, p1)
            else:
                award_elo(p1, p2, definitive=0.5)

    if turn_limit:
        award_elo(p1, p2, definitive=0.5)


def to_readable(val):
    index = [
        ("Alg", "", True),
        ("MC", "mcts", True),
        ("MP", "mcts", False),
        ("T (s)", "", True),
        ("FMU", "mcts", False),
        ("Eval", "mm", True)
    ]
    vals = val.split("|")
    algorithm = vals[0]
    result = []
    for value, tup in zip(vals, index):
        if tup[1] in algorithm and (tup[2] or value != "0"):
            result.append(tup[0] + ": " + value)
    return ",".join(result)


def print_ratings(fig=False):
    items = sorted(data.items(), key=lambda x: x[1])
    for key, val in items:
        print(key, val)


    if not fig:
        return
#     items = """random|1|0|1.0|0 1200.1568605698535
# mcts|1|0|1.0|1 1405.4712611696475
# mcts|5|1|1.0|0 1421.7182039261268
# mcts|1|1|1.0|0 1425.2472175736812
# mcts|100|0|1.0|0 1429.8761968221124
# mcts|5|0|1.0|0 1443.516876720429
# mcts|1|0|1.0|0 1449.7531116985722
# mcts|5|0|5|0 1498.1127864937312
# mcts|1|0|5|0 1538.957255408226
# mm|1|0|1.0|0 1680.9590757748765
# mm|1|0|0.01|0 1726.2501081644432
# mm|1|0|0.1|0 1779.9810456783"""
#     items = [item.split(" ") for item in items.split("\n")]
#     for item in items:
#         item[1] = float(item[1])
    import matplotlib.pyplot as plt

    fig, ax = plt.subplots()
    ax.barh([to_readable(item[0]) for item in items], [item[1] for item in items])
    ax.set_xlim((1000, 2000))
    ax.vlines(1500, -0.5, len(items) - 0.5, colors="red")
    ax.set_xlabel("ELO Rating")
    ax.set_ylabel("Algorithm variants")
    plt.tight_layout()
    plt.show()

def main():
    global data
    if os.path.exists("data.store"):
        data = pickle.load(open("data.store", "rb"))

    for key, val in list(data.items()):
        if key.endswith("|2"):
            del data[key]

    print("Initial ratings")
    print_ratings(fig=True)

    pool = [
        Configuration("random"),
        # Configuration("mcts", 1, t=25),
        # Configuration("mcts", 1, t=125),

        Configuration("mm"),
        Configuration("mm", t=0.1),
        Configuration("mm", t=0.01),
        Configuration("mcts", 1),
        # Configuration("mcts", 5),
        # Configuration("mcts", 1, t=5),
        # Configuration("mcts", 100),
        Configuration("mcts", 1, True),
        # Configuration("mcts", 5, True),
        Configuration("mcts", 1, False, f=True),
    ]

    for i in range(5):
        for a, b in itertools.combinations(pool, 2):
            # Ensure fair playout by letting them play both sides
            playout(a, b)
            playout(b, a)

            print_ratings(fig=False)
        pickle.dump(data, open("data.store", "wb"))

    ndata = defaultdict(dd)
    strpool = [str(p) for p in pool]
    for k, v in data.items():
        if k in strpool:
            ndata[k] = v
    data = ndata
    print_ratings(fig=True)


if __name__ == "__main__":
    main()

# TODO:
#  Remove draws by assigning a win at the end somehow
#     Using current heuristic does not improve the search, so its doubtful that this will work here.

# TODO:
#  More aggressively prioritize moves   ->   No difference in results
#     random|1|0|1.0 1334.226796807779
#     mcts|1|1|1.0 1440.0386936507145
#     mcts|1|0|1.0 1462.3974880832889
#     mm|1|0|0.01 1650.2480193208003

# TODO:
#  Find different MCTS enhancements
#     First play urgency  -  Use heuristic to prioritize exploring certain nodes  ->  No impact
#     Transpositions      -  Transposition tables
#     Search seeding      -  Initialize node results based on heuristic
#  Except for transposition tables they have the issue that a heuristic needs to work.

# TODO:
#  Find some other heuristic to guide the search


# INITIAL Ratings
# random|1|0|1.0 1313.0014414434038
# mcts|5|1|1.0 1421.7182039261268
# mcts|100|0|1.0 1429.8761968221124
# mcts|5|0|1.0 1443.516876720429
# mcts|1|1|1.0 1455.8040045988662
# mcts|1|0|1.0 1460.2138698749352
# mcts|5|0|5 1498.1127864937312
# mm|1|0|1.0 1626.0267329847377
# mm|1|0|0.01 1664.4658238897173
# mm|1|0|0.1 1687.26406324594


# After time fix ratings
# random|1|0|1.0|0 1200.1568605698535
# mcts|1|0|1.0|1 1405.4712611696475
# mcts|5|1|1.0|0 1421.7182039261268
# mcts|1|1|1.0|0 1425.2472175736812
# mcts|100|0|1.0|0 1429.8761968221124
# mcts|5|0|1.0|0 1443.516876720429
# mcts|1|0|1.0|0 1449.7531116985722
# mcts|5|0|5|0 1498.1127864937312
# mcts|1|0|5|0 1538.957255408226
# mm|1|0|1.0|0 1680.9590757748765
# mm|1|0|0.01|0 1726.2501081644432
# mm|1|0|0.1|0 1779.9810456783


# No transposition table  (5 additional runs)
# random|1|0|1.0|0 1200.1568605698535
# mcts|1|0|1.0|1 1405.4712611696475
# mcts|5|1|1.0|0 1421.7182039261268
# mcts|1|1|1.0|0 1425.2472175736812
# mcts|100|0|1.0|0 1429.8761968221124
# mcts|5|0|1.0|0 1443.516876720429
# mcts|1|0|1.0|0 1449.7531116985722
# mcts|5|0|5|0 1498.1127864937312
# mcts|1|0|5|0 1538.957255408226
# mm|1|0|1.0|0 1722.4702429740328
# mm|1|0|0.01|0 1729.1895884646922
# mm|1|0|0.1|0 1735.5303981788948