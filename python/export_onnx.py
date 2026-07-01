#!/usr/bin/env python3
"""Export best model to ONNX after training."""
import torch
from pathlib import Path
import json

# Load best checkpoint
ckpt_path = Path('models/test_ode/best_model.pth')
ckpt = torch.load(ckpt_path, map_location='cpu')

# Load hyperparameters
with open('models/test_ode/hparams.json') as f:
    hparams = json.load(f)

# Reconstruct model - infer input/output dim from checkpoint
from train import GeodesicMLP
# Checkpoint state_dict keys: first layer weight shape determines input_dim
first_layer_key = [k for k in ckpt['model_state_dict'].keys() if 'weight' in k][0]
first_layer_weight = ckpt['model_state_dict'][first_layer_key]
input_dim = first_layer_weight.shape[1]
output_dim = 4  # final_event

model = GeodesicMLP(
    input_dim=input_dim,
    hidden_dim=hparams['hidden_dim'],
    output_dim=output_dim,
    num_layers=hparams['num_layers']
)
model.load_state_dict(ckpt['model_state_dict'])
model.eval()

# Get sample input from validation set
from train import GeodesicDataset
val_dataset = GeodesicDataset(Path('data/test_1k.h5'), split='val')
sample_input, _ = val_dataset[0]
sample_input = torch.tensor(sample_input, dtype=torch.float32).unsqueeze(0)

# Export to ONNX
onnx_path = Path('models/test_ode/geodesic_ode.onnx')
torch.onnx.export(
    model, sample_input, onnx_path,
    export_params=True, opset_version=14, do_constant_folding=True,
    input_names=['input'], output_names=['final_event'],
    dynamic_axes={'input': {0: 'batch_size'}, 'final_event': {0: 'batch_size'}}
)
print(f"ONNX export successful: {onnx_path}")
print(f"File size: {onnx_path.stat().st_size / 1024:.1f} KB")
