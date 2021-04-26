import matplotlib.pyplot as plt




def main():
    g = 101
    random = [84, 14, 62, g, g, 68, 88, g, 62, g, g, g, 46, 46, g, g, 62, g, g, 24, 90, 54, g, g, 80, 52, 64, 93, 38,
              g, 86, g, 46, 34, g, 74, 50, 68, 22, g, 84, 12, g, 76, g, g, 72, g, 14, g, 70, g, 76, 92, 70, g, g, 54,
              g, 82]
    uct1dot4 = [72, g, g, g, 32, 76, 68, g, g, 64, g, 54, g, 27, g, 84, g, g, 44, 71, g, g, g, 46, g, 94, 88, 56, 54,
                58,
                90, g, g, g, 96, 97, 54, 16, 34, 20, g, 99, 26, g, g, g, 85, 38, 68, g, g, 66, 20, 62, 56, g, 36, g, g,
                32, g, g, 28, 58, g, 91, 68, g, 60, g, 12]

    uct100 = [g, 74, g, g, 76, 58, 46, 32, g, 74, 35, g, g, g, 78, g, g, g, 30, g, g, g, 46, 60, g, 40, g, g, 24, g, g,
              61, g, 24, g, 80, 34, 38, 20, g, 70, g, 77, 82, g, g, g, g, 64]

    mm_allmoves = [88, 26, 19, 46, 58, 58, 16, 40, 98, 26, 30, 48, 28, 36, 78, 24, 50, 79, 34, 59, 32, 52, 26, 56, 78,
                   72, 48, 40, 18, 36, 86, 44, 22]

    mm_noants = [32, 32, 52, 28, 30, 48, 23, 24, 24, 42, 34, 58, 24, 22, 30, 30, 22, 22, 42, 40, 26, 32, 42, 30, 23, 86,
                 24, 54, 34, 38, 90, 20, 38, 30, 22, 48, 32, 30, 30, 36, 58, 24, 50]

    mcts_uct100_5sec = [22,56,97,26,15,58,38,50,24,72,46,76,22,16,13,42,24,28,34,24,24,23,64,96,18,32,14,80,56]

    mm_noants_5sec = [16,32,34,34,36,38,26,32,72,22,72,26,34,62,28,34,32,26,22,24,22,30,20,52,26,50,28,26,36]

    mm_20sec = [40,23,32,28,26,16,40,36,51,18,25,22,46,24,38,42,24,37,34,34,34,28,16,14,32,36,30,62,28,44,20,84,26]

    mcts_20sec = [26,36,24,30,72,26,14,26,30,88,48,28,99,14,26,28,52,36,34,85,20,98,92,26,17,88,28,36,58,69,48,62,26,21,40,70,26,42,26,46,58,54,50,18]

    random = list(filter(lambda x: x != g, random))
    uct1dot4 = list(filter(lambda x: x != g, uct1dot4))
    uct100 = list(filter(lambda x: x != g, uct100))

    print(len(random), sum(random) / len(random))
    print(len(uct1dot4), sum(uct1dot4) / len(uct1dot4))
    print(len(uct100), sum(uct100) / len(uct100))
    print(len(mm_allmoves), sum(mm_allmoves) / len(mm_allmoves))
    print(len(mm_noants), sum(mm_noants) / len(mm_noants))
    print(len(mcts_uct100_5sec), sum(mcts_uct100_5sec) / len(mcts_uct100_5sec))
    print(len(mm_noants_5sec), sum(mm_noants_5sec) / len(mm_noants_5sec))
    print(len(mm_20sec), sum(mm_20sec) / len(mm_20sec))
    print(len(mcts_20sec), sum(mcts_20sec) / len(mcts_20sec))

    plt.hist([random, uct1dot4, uct100, mm_allmoves, mm_noants], bins=5)
    plt.legend(["Random", "UCT c=1.4", "UCT c=100", "MM Full", "MM No ants"])
    plt.show()

    plt.hist([mm_noants_5sec, mcts_uct100_5sec, mm_20sec, mcts_20sec], bins=5)
    plt.legend(["MCTS UCT c=100 5 sec", "MM no ants 5 sec", "MM 20 sec", "MCTS 20 sec"])
    plt.show()

if __name__ == "__main__":
    main()


"""
 () - () - ()
 
     0 1 0
 M = 1 0 1
     0 1 0
  
 v = [1 0 0]
 
 v * M = [0 1 0] = v2
 
 v = v + v2 = [1 1 0]
 
 v * M = [1 1 1] = v3
 
 
 
 
 



"""