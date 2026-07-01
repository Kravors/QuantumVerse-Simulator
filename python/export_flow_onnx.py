#!/usr/bin/env python3
"""
Task 2.5: Export trained normalizing flow to ONNX format.

Usage:
    python export_flow_onnx.py --checkpoint models/curvature_flow/best_model.pth \
                               --metadata models/curvature_flow/metadata.yaml \
                               --output models/curvature_flow/curvature_flow.onnx
"""

import argparse
import os
import sys
from pathlib import Path

import torch
import torch.nn as nn
import yaml

# Import the model definition (use simple implementation if nflows unavailable)
try:
    from nflows import flows, transforms, distributions
    HAS_NFLOWS = True
except ImportError:
    HAS_NFLOWS = False
    # Import from train script (reuse SimpleRealNVP)
    # We'll re-define here for standalone export
    print("Warning: nflows not available, using simple RealNVP implementation")

    class SimpleCouplingLayer(nn.Module):
        """Simple affine coupling layer for RealNVP."""
        def __init__(self, dim, hidden_dim=128, mask_type='alternate'):
            super().__init__()
            self.dim = dim
            self.hidden_dim = hidden_dim
            if mask_type == 'alternate':
                mask = torch.zeros(dim)
                mask[::2] = 1
            else:
                mask = torch.ones(dim)
                mask[::2] = 0
            self.register_buffer('mask', mask)
            self.net = nn.Sequential(
                nn.Linear(int(dim * (1 - mask.sum().item())), hidden_dim),
                nn.ReLU(),
                nn.Linear(hidden_dim, hidden_dim),
                nn.ReLU(),
                nn.Linear(hidden_dim, dim)
            )
        def forward(self, x, reverse=False):
            masked_x = x * self.mask
            unchanged_dim = int(self.mask.sum().item())
            changed_dim = self.dim - unchanged_dim
            if changed_dim == 0:
                return x, torch.zeros(x.shape[0], device=x.device)
            net_input = x * (1 - self.mask)
            net_output = self.net(net_input)
            scale = torch.exp(net_output[:, :changed_dim])
            shift = net_output[:, changed_dim:changed_dim*2] if changed_dim*2 <= net_output.shape[1] else net_output[:, changed_dim:]
            if reverse:
                y = masked_x + (1 - self.mask) * (x - shift) / scale
                log_det = -torch.log(scale).sum(dim=1)
            else:
                y = masked_x + (1 - self.mask) * (scale * (1 - self.mask) * x + shift)
                log_det = torch.log(scale).sum(dim=1)
            return y, log_det

    class SimpleRealNVP(nn.Module):
        """Stack of coupling layers."""
        def __init__(self, dim, n_layers=4, hidden_dim=128):
            super().__init__()
            self.dim = dim
            self.n_layers = n_layers
            layers = []
            for i in range(n_layers):
                mask_type = 'alternate' if i % 2 == 0 else 'reverse'
                layers.append(SimpleCouplingLayer(dim, hidden_dim, mask_type))
            self.layers = nn.ModuleList(layers)
            self.base_dist = torch.distributions.MultivariateNormal(
                torch.zeros(dim), torch.eye(dim)
            )
        def forward(self, x, reverse=False):
            log_det_total = torch.zeros(x.shape[0], device=x.device)
            y = x
            if not reverse:
                for layer in self.layers:
                    y, log_det = layer(y, reverse=False)
                    log_det_total += log_det
            else:
                for layer in reversed(self.layers):
                    y, log_det = layer(y, reverse=True)
                    log_det_total += log_det
            return y, log_det_total
        def log_prob(self, x):
            z, log_det = self.forward(x, reverse=False)
            log_prob_base = self.base_dist.log_prob(z)
            return log_prob_base + log_det
        def sample(self, n):
            z = self.base_dist.sample((n,))
            x, _ = self.forward(z, reverse=True)
            return x

def load_model(checkpoint_path, metadata):
    """Load trained flow model from checkpoint."""
    feature_dim = metadata['feature_dim']
    model_type = metadata.get('model_type', 'realnvp')
    n_layers = metadata.get('n_layers', 4)
    hidden_dim = metadata.get('hidden_dim', 128)
    
    if model_type == 'realnvp':
        if HAS_NFLOWS:
            # Build nflows model
            transform = transforms.CompositeTransform([
                transforms.AffineCouplingTransform(
                    mask=torch.arange(feature_dim) % 2 == 0,
                    transform_net=lambda d: nn.Sequential(
                        nn.Linear(d // 2, hidden_dim),
                        nn.ReLU(),
                        nn.Linear(hidden_dim, hidden_dim),
                        nn.ReLU(),
                        nn.Linear(hidden_dim, d)
                    )
                ) for _ in range(n_layers)
            ])
            model = flows.Flow(transform, distributions.StandardNormal([feature_dim]))
        else:
            model = SimpleRealNVP(feature_dim, n_layers=n_layers, hidden_dim=hidden_dim)
    else:
        raise ValueError(f"Unsupported model type: {model_type}")
    
    # Load checkpoint
    print(f"Loading checkpoint from {checkpoint_path}...")
    checkpoint = torch.load(checkpoint_path, map_location='cpu')
    model.load_state_dict(checkpoint['model_state_dict'])
    model.eval()
    
    print(f"Model loaded: epoch {checkpoint.get('epoch', 'unknown')}, "
          f"val_loss={checkpoint.get('val_loss', 'unknown'):.4f}")
    
    return model

def export_to_onnx(model, output_path, feature_dim, device='cpu'):
    """Export PyTorch model to ONNX format."""
    print(f"Exporting to ONNX: {output_path}")
    
    # Create dummy input
    dummy_input = torch.randn(1, feature_dim, device=device)
    
    # Ensure model is on correct device
    model = model.to(device)
    model.eval()
    
    # Export
    try:
        torch.onnx.export(
            model,
            dummy_input,
            output_path,
            export_params=True,
            opset_version=18,
            do_constant_folding=True,
            input_names=['input'],
            output_names=['log_prob'],
            dynamic_axes={
                'input': {0: 'batch_size'},
                'log_prob': {0: 'batch_size'}
            }
        )
        print(f"ONNX export successful: {output_path}")
        
        # Verify
        import onnx
        onnx_model = onnx.load(output_path)
        onnx.checker.check_model(onnx_model)
        print("ONNX model verification passed")
        
        # Print info
        print(f"  Input: {[d.dim_value for d in onnx_model.graph.input[0].type.tensor_type.shape.dim]}")
        print(f"  Output: {[d.dim_value for d in onnx_model.graph.output[0].type.tensor_type.shape.dim]}")
        
        return True
    except Exception as e:
        print(f"ONNX export failed: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description='Export normalizing flow to ONNX')
    parser.add_argument('--checkpoint', type=str, required=True,
                        help='Path to best_model.pth checkpoint')
    parser.add_argument('--metadata', type=str, required=True,
                        help='Path to metadata.yaml')
    parser.add_argument('--output', type=str, required=True,
                        help='Output ONNX file path')
    parser.add_argument('--device', type=str, default='cpu',
                        help='Device for export (cpu or cuda)')
    args = parser.parse_args()
    
    # Load metadata
    with open(args.metadata, 'r') as f:
        metadata = yaml.safe_load(f)
    
    feature_dim = metadata['feature_dim']
    print(f"Feature dimension: {feature_dim}")
    print(f"Model type: {metadata.get('model_type', 'realnvp')}")
    print(f"Layers: {metadata.get('n_layers', 4)}")
    print(f"Hidden dim: {metadata.get('hidden_dim', 128)}")
    
    # Load model
    model = load_model(args.checkpoint, metadata)
    
    # Ensure output directory exists
    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    
    # Export
    success = export_to_onnx(model, args.output, feature_dim, device=args.device)
    
    if success:
        print("\nExport complete!")
        print(f"Model saved to: {args.output}")
        
        # Save export metadata
        export_meta = {
            'original_checkpoint': args.checkpoint,
            'feature_dim': feature_dim,
            'model_type': metadata.get('model_type', 'realnvp'),
            'n_layers': metadata.get('n_layers', 4),
            'hidden_dim': metadata.get('hidden_dim', 128),
            'normalization': {
                'mean': metadata.get('mean', []),
                'std': metadata.get('std', []),
            },
            'opset_version': 18,
        }
        import json
        with open(args.output.replace('.onnx', '_metadata.json'), 'w') as f:
            json.dump(export_meta, f, indent=2)
        print(f"Metadata saved to: {args.output.replace('.onnx', '_metadata.json')}")
    else:
        print("\nExport failed!")
        sys.exit(1)

if __name__ == '__main__':
    main()
