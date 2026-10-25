# QuantumVerse RL Discovery Environment
# Gym environment for autonomous black hole parameter exploration

import gymnasium as gym
from gymnasium import spaces
import numpy as np
from typing import Tuple, Dict, Any


class QuantumVerseEnv(gym.Env):
    """
    RL environment for discovering new physics in black hole parameter space.

    State: [Mass, Spin, Charge, Anomaly Score]
    Action: [delta_mass, delta_spin, delta_charge]

    The agent explores (M, a, Q) space, avoiding naked singularities
    (cosmic censorship violation: a^2 + Q^2 > M^2) and seeking extremal
    black holes (a^2 + Q^2 ≈ M^2).
    """

    metadata = {"render_modes": []}

    def __init__(self):
        super(QuantumVerseEnv, self).__init__()

        # Observation space: Mass [0.1, 100], Spin [0, 1.5], Charge [0, 1.5], Score [-1, 10]
        self.observation_space = spaces.Box(
            low=np.array([0.1, 0.0, 0.0, -1.0], dtype=np.float32),
            high=np.array([100.0, 1.5, 1.5, 10.0], dtype=np.float32),
            dtype=np.float32,
        )

        # Action space: Deltas applied to params (scaled per-dimension)
        self.action_space = spaces.Box(
            low=np.array([-0.1, -0.1, -0.1], dtype=np.float32),
            high=np.array([0.1, 0.1, 0.1], dtype=np.float32),
            dtype=np.float32,
        )

        self.max_steps = 50
        self.current_step = 0
        self.state = np.zeros(4, dtype=np.float32)

    def reset(
        self, seed: int = None, options: Dict[str, Any] = None
    ) -> Tuple[np.ndarray, Dict[str, Any]]:
        super().reset(seed=seed)
        self.current_step = 0
        # Start with a standard Schwarzschild black hole
        self.state = np.array([10.0, 0.0, 0.0, 0.0], dtype=np.float32)
        return self.state, {}

    def step(
        self, action: np.ndarray
    ) -> Tuple[np.ndarray, float, bool, bool, Dict[str, Any]]:
        self.current_step += 1

        # 1. Apply action (scaled deltas)
        self.state[0] += action[0] * 10.0  # Mass change scaled up
        self.state[1] += action[1] * 1.0   # Spin
        self.state[2] += action[2] * 1.0   # Charge

        # 2. Clamp to physical limits
        M = float(np.clip(self.state[0], 0.1, 100.0))
        a = float(np.clip(self.state[1], 0.0, 1.5))
        Q = float(np.clip(self.state[2], 0.0, 1.5))

        self.state[0] = M
        self.state[1] = a
        self.state[2] = Q

        # 3. Check for Cosmic Censorship Violation (Naked Singularity)
        # In GR, a^2 + Q^2 <= M^2 must hold for a black hole.
        if (a**2 + Q**2) > (M**2):
            reward = -10.0
            terminated = True
            self.state[3] = -1.0  # Invalid physics marker
        else:
            # 4. Calculate "Extremality" (Anomaly Score)
            extremality = (a**2 + Q**2) / (M**2)
            self.state[3] = extremality

            reward = -0.1  # Small step penalty to encourage speed
            terminated = False

            # Reward for finding an Extremal Black Hole
            if extremality > 0.99:
                reward = 100.0
                terminated = True

        truncated = self.current_step >= self.max_steps

        info = {
            "mass": M,
            "spin": a,
            "charge": Q,
            "extremality": self.state[3],
            "step": self.current_step,
        }

        return self.state, reward, terminated, truncated, info

    def render(self):
        pass

    def close(self):
        pass
