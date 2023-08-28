from games.connect4.connect4 import Connect4


def test_trainer():
    game = Connect4()
    while game.turn != game.turn_limit:
        children = list(game.children())
        game.select_child(children[0])
        game.print()
        if game.winner() is not None:
            print(game.winner())
            break


if __name__ == "__main__":
    test_trainer()
