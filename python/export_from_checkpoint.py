#!/usr/bin/env python3
"""
QuantumVerse - Task 2.1: Export Best Model to ONNX

This script loads the best trained checkpoint and exports it to ONNX.
Use when training completed but ONNX export failed (e.g., Windows Unicode issue).

Author: Kilo Code (AI Assistant)
Date: 2026-05-04
"""

import torch
import sys
import io
from pathlib import Path

# Import the model class from train.py
# We need to import GeodesicMLP to correctly load the state_dict
import importlib.util
spec = importlib.util.spec_from_file_location("train", Path(__file__).parent / "train.py")
train_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(train_module)
GeodesicMLP = train_module.GeodesicMLP

def export_best(checkpoint_path: str, output_dir: str):
    """Load best model and export to ONNX with Windows-safe encoding."""
    checkpoint_path = Path(checkpoint_path)
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Load checkpoint
    print(f"Loading checkpoint: {checkpoint_path}")
    checkpoint = torch.load(checkpoint_path, map_location='cpu')
    state_dict = checkpoint['model_state_dict']
    val_loss = checkpoint['val_loss']
    print(f"Checkpoint val_loss: {val_loss:.6f}")

    # Determine dimensions from checkpoint metadata or state_dict
    # state_dict keys are like: network.0.weight, network.0.bias, network.2.weight, ...
    # Input dim from first layer weight shape [hidden, input]
    first_weight_key = [k for k in state_dict.keys() if 'weight' in k and '0' in k][0]
    input_dim = state_dict[first_weight_key].shape[1]
    hidden_dim = state_dict[first_weight_key].shape[0]
    # Output dim from last layer bias (network.6.bias for 3-layer MLP)
    last_bias_key = [k for k in state_dict.keys() if 'bias' in k and ('6' in k or '9' in k)][-1]
    output_dim = state_dict[last_bias_key].shape[0]

    print(f"Model dimensions: input={input_dim}, hidden={hidden_dim}, output={output_dim}")

    # Reconstruct model using GeodesicMLP class
    model = GeodesicMLP(input_dim=input_dim, hidden_dim=hidden_dim, output_dim=output_dim, num_layers=3)
    model.load_state_dict(state_dict)
    model.eval()
    print("Model loaded and set to eval mode.")

    # Create dummy input (batch=1)
    dummy_input = torch.randn(1, input_dim)

    # Export with Windows-safe stdout (redirect to UTF-8 to handle torch.onnx checkmarks)
    onnx_path = output_dir / 'geodesic_ode.onnx'
    old_stdout = sys.stdout
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
    try:
        torch.onnx.export(
            model,
            dummy_input,
            str(onnx_path),
            export_params=True,
            opset_version=14,
            do_constant_folding=True,
            input_names=['input'],
            output_names=['final_event'],
            dynamic_axes={
                'input': {0: 'batch_size'},
                'final_event': {0: 'batch_size'}
            }
        )
        print(f"ONNX export successful: {onnx_path}")
    except Exception as e:
        print(f"ONNX export failed: {e}", file=sys.stderr)
        raise
    finally:
        sys.stdout = old_stdout

    # Save metadata
    import json
    from datetime import datetime
    metadata = {
        'val_error': float(val_loss),
        'input_dim': int(input_dim),
        'output_dim': int(output_dim),
        'hidden_dim': int(hidden_dim),
        'num_layers': 3,
        'date_trained': datetime.now().isoformat(),
        'checkpoint': str(checkpoint_path)
    }
    with open(output_dir / 'metadata.json', 'w') as f:
        json.dump(metadata, f, indent=2)
    print(f"Metadata saved: {output_dir / 'metadata.json'}")

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Export best model to ONNX')
    parser.add_argument('--checkpoint', type=str, default='models/geodesic_ode/best_model.pth',
                        help='Path to best_model.pth checkpoint')
    parser.add_argument('--output-dir', type=str, default='models/geodesic_ode',
                        help='Output directory for ONNX model')
    args = parser.parse_args()
    export_best(args.checkpoint, args.output_dir)
