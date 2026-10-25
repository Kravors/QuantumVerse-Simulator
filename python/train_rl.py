# QuantumVerse RL Discovery Agent Training
# Trains PPO agent to discover extremal black holes

from stable_baselines3 import PPO
from discovery_env import QuantumVerseEnv
import numpy as np

# Create environment
env = QuantumVerseEnv()

# Initialize PPO agent
model = PPO(
    "MlpPolicy",
    env,
    verbose=1,
    learning_rate=0.0003,
    n_steps=2048,
    batch_size=64,
    tensorboard_log=None,
)

print("Starting RL Discovery Training...")
print("Goal: Find extremal black holes (a^2 + Q^2 ~= M^2) without naked singularities")
print()

# Train for 100k steps
model.learn(total_timesteps=100_000)

# Save the agent
model.save("rl_discovery_agent")
print("\nAgent saved to rl_discovery_agent.zip")

# Quick evaluation: run a few episodes to verify learning
print("\n=== Quick Evaluation ===")
eval_env = QuantumVerseEnv()
obs, _ = eval_env.reset()
total_reward = 0
for step in range(50):
    action, _ = model.predict(obs, deterministic=True)
    obs, reward, terminated, truncated, info = eval_env.step(action)
    total_reward += reward
    if terminated or truncated:
        print(
            f"Episode ended at step {step}: "
            f"M={info['mass']:.2f}, a={info['spin']:.2f}, Q={info['charge']:.2f}, "
            f"extremality={info['extremality']:.4f}, reward={total_reward:.1f}"
        )
        obs, _ = eval_env.reset()
        total_reward = 0
