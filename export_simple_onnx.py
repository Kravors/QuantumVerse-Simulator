#!/usr/bin/env python3
"""
Export trained SimpleRealNVP model to ONNX.
"""

import torch
import sys
sys.path.insert(0, 'python')
from train_curvature_flow import SimpleRealNVP
import yaml
import os

class FlowWrapper(torch.nn.Module):
    """Wrapper to expose only log_prob for ONNX export."""
    def __init__(self, flow):
        super().__init__()
        self.flow = flow

    def forward(self, x):
        log_prob = self.flow.log_prob(x)
        return log_prob.unsqueeze(1)  # (batch, 1)

def main():
    checkpoint_path = 'models/curvature_flow/best_model.pth'
    metadata_path = 'models/curvature_flow/metadata.yaml'
    output_path = 'models/curvature_flow/curvature_flow.onnx'

    # Load metadata
    with open(metadata_path, 'r') as f:
        metadata = yaml.safe_load(f)

    feature_dim = metadata['feature_dim']
    n_layers = metadata['n_layers']
    hidden_dim = metadata['hidden_dim']

    print(f"Loading model: {feature_dim} features, {n_layers} layers, {hidden_dim} hidden")

    # Create model
    model = SimpleRealNVP(dim=feature_dim, n_layers=n_layers, hidden_dim=hidden_dim)

    # Load checkpoint
    checkpoint = torch.load(checkpoint_path, map_location='cpu')
    # Use strict=False because old checkpoints used MultivariateNormal base_dist
    # while the current code uses StandardNormal (buffers have correct defaults)
    model.load_state_dict(checkpoint['model_state_dict'], strict=False)
    model.eval()

    print(f"Model loaded from epoch {checkpoint['epoch']}, val_loss={checkpoint['val_loss']:.4f}")

    # Wrap for export
    wrapped = FlowWrapper(model)
    wrapped.eval()

    # Dummy input for export
    batch_size = 1
    dummy_input = torch.randn(batch_size, feature_dim)

    # Export to ONNX
    print(f"Exporting to ONNX: {output_path}")
    torch.onnx.export(
        wrapped,
        dummy_input,
        output_path,
        input_names=['features'],
        output_names=['log_prob'],
        opset_version=17,
        do_constant_folding=True,
        verbose=False
    )

    print("ONNX export successful!")

    # Also export metadata as JSON for C++ side
    import json
    meta_json = {
        'feature_dim': feature_dim,
        'model_type': 'SimpleRealNVP',
        'n_layers': n_layers,
        'hidden_dim': hidden_dim,
        'mean': metadata['mean'],
        'std': metadata['std']
    }
    meta_json_path = 'models/curvature_flow/metadata.json'
    with open(meta_json_path, 'w') as f:
        json.dump(meta_json, f, indent=2)
    print(f"Metadata saved to {meta_json_path}")

if __name__ == '__main__':
    main()
