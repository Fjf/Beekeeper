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

    random = list(filter(lambda x: x != g, random))
    uct1dot4 = list(filter(lambda x: x != g, uct1dot4))
    uct100 = list(filter(lambda x: x != g, uct100))

    print(sum(random) / len(random))
    print(sum(uct1dot4) / len(uct1dot4))
    print(sum(uct100) / len(uct100))

    plt.hist([random, uct1dot4, uct100], bins=5)
    plt.legend(["Random", "UCT c=1.4", "UCT c=100"])
    plt.show()


if __name__ == "__main__":
    main()
