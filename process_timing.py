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


def parsenice():
    data = """/var/scratch/romein/ais00X-data/ais002-data/data/SB250-202106281653-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais002-data/data/SB250-202106291325-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais002-data/data/SB250-202106291501-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais002-data/data/SB250-202106291602-lba_outer.vis
/var/scratch/romein/ais00X-data/ais002-data/data/SB250-202106291649-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais002-data/data/SB250-202106291812-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais002-data/data/SB250-202106291913-lba_outer.vis
/var/scratch/romein/ais00X-data/ais002-data/data/SB250-202106292200-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais002-data/data/SB250-202106292301-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais002-data/data/SB250-202106300202-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais003-data/data/SB267-202106281653-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais003-data/data/SB267-202106291325-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais003-data/data/SB267-202106291501-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais003-data/data/SB267-202106291602-lba_outer.vis
/var/scratch/romein/ais00X-data/ais003-data/data/SB267-202106291649-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais003-data/data/SB267-202106291812-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais003-data/data/SB267-202106291913-lba_outer.vis
/var/scratch/romein/ais00X-data/ais003-data/data/SB267-202106292200-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais003-data/data/SB267-202106292301-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais003-data/data/SB267-202106300202-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais003-data/data/SB267-202106300303-lba_outer.vis
/var/scratch/romein/ais00X-data/ais003-data/data/SB267-202106300505-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais003-data/data/SB267-202106300606-lba_outer.vis
/var/scratch/romein/ais00X-data/ais003-data/data/SB267-202106300734-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais003-data/data/SB267-202106300840-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais004-data/data/SB278-202106281653-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais004-data/data/SB278-202106291501-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais004-data/data/SB278-202106291602-lba_outer.vis
/var/scratch/romein/ais00X-data/ais004-data/data/SB278-202106291649-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais004-data/data/SB278-202106291812-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais004-data/data/SB278-202106291913-lba_outer.vis
/var/scratch/romein/ais00X-data/ais004-data/data/SB278-202106292200-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais004-data/data/SB278-202106292301-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais004-data/data/SB278-202106300202-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais004-data/data/SB278-202106300303-lba_outer.vis
/var/scratch/romein/ais00X-data/ais005-data/data/SB299-202106291501-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais005-data/data/SB299-202106291602-lba_outer.vis
/var/scratch/romein/ais00X-data/ais005-data/data/SB299-202106291649-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais005-data/data/SB299-202106291812-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais005-data/data/SB299-202106291913-lba_outer.vis
/var/scratch/romein/ais00X-data/ais005-data/data/SB299-202106292200-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais005-data/data/SB299-202106292301-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais005-data/data/SB299-202106300202-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais005-data/data/SB299-202106300303-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106281653-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106291501-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106291602-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106291649-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106291812-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106291913-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106292200-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106292301-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106300202-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106300303-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106300505-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106300606-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106300734-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106300840-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106301151-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202106301339-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202107010253-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202107010525-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202107010706-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202107010807-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202107010817-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202107011629-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB322-202107011749-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106281653-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106291325-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106291501-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106291602-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106291649-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106291812-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106291913-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106292200-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106292301-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106300202-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106300303-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106300505-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106300606-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106300734-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106300840-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106301151-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202106301339-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202107010253-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202107010525-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202107010706-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202107010807-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202107010817-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202107011629-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB327-202107011749-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106281653-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106291325-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106291501-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106291602-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106291649-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106291812-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106291913-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106292200-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106292301-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106300202-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106300303-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106300505-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106300606-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106300734-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106300840-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106301151-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202106301339-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202107010253-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202107010525-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202107010706-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202107010807-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202107010817-lba_outer.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202107011629-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais006-data/data/SB330-202107011749-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106281653-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106291501-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106291602-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106291649-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106291812-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106291913-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106292200-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106292301-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106300202-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106300303-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106300505-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106300606-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106300734-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106300840-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106301151-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202106301339-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202107010253-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202107010525-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202107010706-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202107010807-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202107010817-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202107011629-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB296-202107011749-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106281653-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106291325-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106291501-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106291602-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106291649-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106291812-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106291913-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106292200-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106292301-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106300202-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106300303-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106300505-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106300606-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106300734-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106300840-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106301151-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202106301339-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202107010253-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202107010525-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202107010706-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202107010807-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202107010817-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202107011629-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB335-202107011749-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106281653-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106291325-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106291501-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106291602-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106291649-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106291812-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106291913-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106292200-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106292301-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106300202-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106300303-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106300505-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106300606-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106300734-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106300840-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106301151-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202106301339-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202107010253-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202107010525-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202107010706-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202107010807-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202107010817-lba_outer.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202107011629-lba_sparse_even.vis
/var/scratch/romein/ais00X-data/ais007-data/data/SB356-202107011749-lba_sparse_even.vis
"""
    hdrlen = len("/var/scratch/romein/ais00X-data/")
    print("\n".join(line[hdrlen:] for line in data.split("\n")))

if __name__ == "__main__":
    parsenice()
    exit(1)
    main()
