#!/usr/bin/env python3
import torch
from pathlib import Path
from train import GeodesicMLP

print("=== ONNX Export Script ===")
ckpt_path = Path('models/geodesic_ode/best_model.pth')
print(f"Loading checkpoint: {ckpt_path}")
ckpt = torch.load(ckpt_path, map_location='cpu')
state_dict = ckpt['model_state_dict']
val_loss = ckpt['val_loss']
print(f'Checkpoint val_loss: {val_loss:.6f}')

# Get dimensions from first and last linear layers
first_w = state_dict['network.0.weight']
input_dim = first_w.shape[1]
hidden_dim = first_w.shape[0]
output_dim = state_dict['network.9.bias'].shape[0]  # last linear layer
print(f'Dimensions: input={input_dim}, hidden={hidden_dim}, output={output_dim}')

# Build model using the actual class
model = GeodesicMLP(input_dim=input_dim, hidden_dim=hidden_dim, output_dim=output_dim, num_layers=3)
model.load_state_dict(state_dict)
model.eval()
print('Model loaded and set to eval.')

# Dummy input
dummy = torch.randn(1, input_dim)
onnx_path = Path('models/geodesic_ode/geodesic_ode.onnx')
print(f'Exporting to ONNX: {onnx_path}')

# Export with Windows UTF-8 stdout fix
import sys, io
old_stdout = sys.stdout
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
try:
    torch.onnx.export(
        model, dummy, str(onnx_path),
        export_params=True, opset_version=14, do_constant_folding=True,
        input_names=['input'], output_names=['final_event'],
        dynamic_axes={'input': {0: 'batch_size'}, 'final_event': {0: 'batch_size'}}
    )
    print('ONNX export SUCCESS!')
except Exception as e:
    print(f'ONNX export FAILED: {e}')
    import traceback
    traceback.print_exc()
    exit(1)
finally:
    sys.stdout = old_stdout

print(f'File size: {onnx_path.stat().st_size} bytes')
print('Done!')
