import os.path

import joblib
import numpy as np
import pytorch_lightning
import torch
from joblib import delayed
from matplotlib import pyplot as plt

from games.Game import Game
from train import School
from utils import setup_parser


def performance(model_dir, game_type, model_type):
    # Load all available models into a list to use later
    n_models = 0
    models = []
    while True:
        filename = f"{model_dir}/iteration_{n_models}.pt"
        if not os.path.exists(filename):
            break
        print(f"Loading model {filename}")
        models.append(model_type())
        models[-1].load_state_dict(torch.load(filename))
        n_models += 1

    # Initialize results matrix
    results = np.zeros((n_models, n_models))

    school = School(game_type, model_type)
    # Create winrate matrix

    tuples = []
    for i in range(n_models):
        for j in range(n_models):
            if i >= j: continue

            tuples.append((i, j))

    def get_data(i, j):
        m1 = model_type()
        m2 = model_type()
        m1.load_state_dict(models[i].state_dict())
        m2.load_state_dict(models[j].state_dict())
        winrate, _ = school.winrate(m1, m2, n_games=100)
        return i, j, winrate

    data = joblib.Parallel(n_jobs=18)(delayed(get_data)(i, j) for i, j in tuples)

    for i, j, winrate in data:
        results[i, j] = winrate
        results[j, i] = 1 - winrate

    print(results)
    plt.imshow(results, interpolation="none")
    plt.savefig("results.png")


if __name__ == "__main__":
    parser = setup_parser()
    args = parser.parse_args()

    # Fetch game and corresponding neural network from subclasses automatically.
    game_type = [s for s in Game.__subclasses__()
                 if s.__name__ == args.game][0]
    network_type = [s for s in pytorch_lightning.LightningModule.__subclasses__()
                    if args.game.lower() in s.__name__.lower()][0]

    performance(args.model_dir, game_type, network_type)
