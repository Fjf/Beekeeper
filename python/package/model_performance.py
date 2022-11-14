import os.path

import numpy as np
import torch
from matplotlib import pyplot as plt

from train import School


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
    for i in range(n_models):
        for j in range(n_models):
            if i >= j: continue

            n_sims = 100
            winrate, _ = school.winrate(models[i], models[j], n_sims)
            print(i, j, winrate)
            results[i, j] = winrate
            results[j, i] = 1 - winrate

    print(results)
    plt.imshow(results, interpolation="none")
    plt.savefig("results.png")

