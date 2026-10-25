#!/usr/bin/env python3
"""Export best model to ONNX after training."""
import torch
from pathlib import Path
import json

# Load best checkpoint
ckpt_path = Path('models/geodesic_ode/best_model.pth')
ckpt = torch.load(ckpt_path, map_location='cpu')

# Handle different checkpoint formats
if 'model_state_dict' in ckpt:
    state_dict = ckpt['model_state_dict']
elif 'model' in ckpt:
    state_dict = ckpt['model']
else:
    state_dict = ckpt

# Load hparams
hparams_path = Path('models/geodesic_ode/hparams.json')
if hparams_path.exists():
    with open(hparams_path) as f:
        hparams = json.load(f)
else:
    hparams = {'hidden_dim': 256, 'num_layers': 3}

# Infer input/output dim from checkpoint state_dict
weight_keys = [k for k in state_dict.keys() if 'weight' in k]
first_layer_weight = state_dict[weight_keys[0]]
input_dim = first_layer_weight.shape[1]
last_layer_weight = state_dict[weight_keys[-1]]
output_dim = last_layer_weight.shape[0]
hidden_dim = hparams.get('hidden_dim', state_dict[weight_keys[1]].shape[0] if len(weight_keys) > 1 else output_dim)
num_layers = hparams.get('num_layers', len(weight_keys) - 1)

print(f"input_dim={input_dim}, hidden_dim={hidden_dim}, num_layers={num_layers}, output_dim={output_dim}")

# Use train.py's GeodesicMLP (includes BatchNorm, matches saved checkpoints)
from train import GeodesicMLP
model = GeodesicMLP(
    input_dim=input_dim,
    hidden_dim=hidden_dim,
    output_dim=output_dim,
    num_layers=num_layers
)

# Handle key prefix: checkpoints may use 'net.' or 'network.' prefix
new_state_dict = {}
for k, v in state_dict.items():
    if k.startswith('net.'):
        new_key = 'network.' + k[4:]  # net.X -> network.X
    else:
        new_key = k
    new_state_dict[new_key] = v

model.load_state_dict(new_state_dict)
model.eval()

# Get sample input from validation set
from train import GeodesicDataset
val_dataset = GeodesicDataset(Path('data/geodesic_test_1k.h5'), split='val')
sample_input, _ = val_dataset[0]
sample_input = torch.tensor(sample_input, dtype=torch.float32).unsqueeze(0)

print(f"Sample input shape: {sample_input.shape}")

# Export to ONNX (single file, no external data)
onnx_path = Path('models/geodesic_ode/geodesic_ode.onnx')
torch.onnx.export(
    model, sample_input, onnx_path,
    export_params=True, opset_version=18, do_constant_folding=True,
    input_names=['input'], output_names=['final_event'],
    dynamic_axes={'input': {0: 'batch_size'}, 'final_event': {0: 'batch_size'}}
)
print(f"ONNX export successful: {onnx_path}")
print(f"File size: {onnx_path.stat().st_size / 1024:.1f} KB")
