import json

import gym
import numpy as np
from ray.rllib.agents.ppo import DEFAULT_CONFIG, PPOTrainer
from ray.tune import register_env

from games.hive.hive import Hive
from games.utils import GameState

QUEEN_REWARD_MULTIPLIER = 1

WON_REWARD = 50
LOST_REWARD = -50


class GymHive(gym.Env):
    def __init__(self, config: dict):
        self.hive = Hive(encoding="absolute")

        self.is_player1 = config.get("player1", True)
        self.action_space = gym.spaces.Box(0, 100, (self.hive.action_space,))
        # self.action_space = gym.spaces.Discrete(nn_action_space)
        self.observation_space = gym.spaces.Box(0, 256, (26, 26), dtype=int)

    def step(self, _: np.ndarray):
        # Overwrite action vector
        action = self.action_space.sample()

        assert sum(np.isnan(action)) == 0, action

        if not self.is_player1:
            action = 1 - action

        ############################
        # Select best action
        ############################
        children = list(self.hive.children())
        encodings = [child.encode() for child in children]

        valid_actions = action[encodings]

        for encoding in encodings:
            if not 0 <= encoding < self.hive.action_space:
                children[encodings.index(encoding)].print()
                raise ValueError("Invalid action")
        total = sum(valid_actions)
        if total <= 0:
            probabilities = None
        else:
            probabilities = valid_actions / total

        choice = np.random.choice(np.arange(len(valid_actions)), 1, p=probabilities)[0]
        child = children[choice]

        ############################
        # Compute reward
        ############################
        self.hive.select_child(child)

        tiles = self.hive.count_queen_test() * QUEEN_REWARD_MULTIPLIER
        reward = tiles

        if self.hive.finished() == GameState.UNDETERMINED:
            self.hive.ai_move("random")

        # # After random move you still get win/loss reward?
        if self.hive.finished() == GameState.WHITE_WON:
            if self.is_player1:
                reward += WON_REWARD
            else:
                reward += LOST_REWARD
        elif self.hive.finished() == GameState.BLACK_WON:
            if self.is_player1:
                reward += LOST_REWARD
            else:
                reward += WON_REWARD

        assert (-60 < reward < 60)
        return self.hive.node.contents.to_np()[0], reward, self.hive.finished() != GameState.UNDETERMINED, {}

    def reset(self):
        self.hive.reinitialize()
        return self.hive.node.contents.to_np()[0]

    def render(self, mode="human"):
        self.hive.print()


def main():
    config = DEFAULT_CONFIG.copy()
    config['num_workers'] = 1
    config['num_gpus'] = 0
    config['num_cpus_per_worker'] = 0
    config["entropy_coeff"] = 0

    register_env("HiveEnv", lambda c: GymHive(c))
    agent = PPOTrainer(config, env='HiveEnv')

    results = []
    episode_data = []
    episode_json = []

    n_epochs = 1000
    for epoch in range(n_epochs):
        # for _ in range(1000):
        #     agent.render()
        #     agent.step(agent.compute_action())

        result = agent.train()
        results.append(result)

        episode = {
            'epoch': epoch,
            'episode_reward_min': result['episode_reward_min'],
            'episode_reward_mean': result['episode_reward_mean'],
            'episode_reward_max': result['episode_reward_max'],
            'episode_len_mean': result['episode_len_mean']
        }

        episode_data.append(episode)
        episode_json.append(json.dumps(episode))

        print(
            f'{epoch:3d}: Min/Mean/Max reward: {result["episode_reward_min"]:8.4f}/{result["episode_reward_mean"]:8.4f}/{result["episode_reward_max"]:8.4f}')


if __name__ == "__main__":
    main()
