from python.package.hive import Hive

if __name__ == "__main__":
    hive = Hive(track_history=True)
    hive.test()

    for child in hive.children():
        np = child.to_np()
