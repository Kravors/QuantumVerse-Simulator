# QuantumVerse RL Agent ONNX Export
# Exports trained PPO Actor policy to ONNX for C++ inference

import torch
import torch.nn as nn
from stable_baselines3 import PPO
import numpy as np

# 1. Load the trained agent
model = PPO.load("rl_discovery_agent")

# 2. Wrap only the actor (mean action output, no distribution sampling)
class ActorWrapper(nn.Module):
    def __init__(self, policy):
        super().__init__()
        self.mlp_extractor = policy.mlp_extractor
        self.action_net = policy.action_net

    def forward(self, obs):
        latent_pi, _ = self.mlp_extractor(obs)
        return self.action_net(latent_pi)

wrapped_model = ActorWrapper(model.policy)
wrapped_model.eval()

# 3. Create dummy input matching your observation space
dummy_input = torch.randn(1, 4)  # [Mass, Spin, Charge, AnomalyScore]

# 4. Export to ONNX (disable dynamo to avoid distribution issues)
torch.onnx.export(
    wrapped_model,
    dummy_input,
    "rl_discovery_agent.onnx",
    export_params=True,
    opset_version=18,
    do_constant_folding=True,
    input_names=['state'],
    output_names=['action'],
    dynamic_axes={'state': {0: 'batch_size'}, 'action': {0: 'batch_size'}},
    dynamo=False,
)
print("Exported rl_discovery_agent.onnx successfully!")

# Verify with ONNX Runtime
import onnxruntime as ort
session = ort.InferenceSession("rl_discovery_agent.onnx")
test_obs = np.array([[10.0, 0.0, 0.0, 0.0]], dtype=np.float32)  # Schwarzschild start
action = session.run(None, {'state': test_obs})
print(f"Verification: state={test_obs[0]} -> action={action[0]}")
