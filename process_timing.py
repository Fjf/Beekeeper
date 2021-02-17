from collections import defaultdict


class Timing(object):
    def __init__(self, name, time):
        self.name = name
        self.time = time
        self.children = []
        self.child = None
        self.parent = None

    def add_child(self, child):
        self.child = child
        # self.children.append(child)
        child.parent = self

    def finish(self, time):
        self.time = time - self.time


def print_time_tree(timing: Timing, depth=0, max_depth=2):
    if depth == max_depth:
        return
    spaces = "   " * depth
    print("%s%s (%s)" % (spaces, timing.name, timing.time))
    for child in timing.children:
        print_time_tree(child, depth + 1)


def print_time_sum(timing: dict, root=None):
    if not root:
        for name, time in timing.items():
            print("%s %.5f" % (name, time))
    else:
        rtime = timing[root]
        for name, time in timing.items():
            print("%s %.5f percent of total time (%.5f)" % (name, (time / rtime) * 100, time / 1e6))


def main():
    files = ["c/out.txt", "c/out2.txt"]

    for file in files:
        cur_func = Timing("__all__", 0)
        timing_sums = defaultdict(float)

        with open(file, "r") as fp:
            lines = fp.readlines()

        for line in lines:
            func, name, end, time = line.split("|")

            if end == '0':
                child = Timing(name, float(time))
                cur_func.add_child(child)
                cur_func = child
            else:
                cur_func.finish(float(time))
                timing_sums[cur_func.name] += cur_func.time
                cur_func = cur_func.parent
                # cur_func.children.pop()  # Temporary remove child

        # print_time_tree(cur_func)
        print("Result for %s" % file)
        print_time_sum(timing_sums, root="minimax")


if __name__ == "__main__":
    main()
