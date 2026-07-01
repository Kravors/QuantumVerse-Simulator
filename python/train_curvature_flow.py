#!/usr/bin/env python3
"""
Task 2.5: Train Normalizing Flow on Curvature Dataset

Trains a normalizing flow (RealNVP or MAF) to learn the distribution
of curvature invariants from "normal" GR spacetimes. The trained model
will be used for anomaly detection: high -log p(x) indicates unusual
curvature patterns (exotic spacetimes, quantum corrections, etc.).

Usage:
    python train_curvature_flow.py --dataset data/curvature_dataset.h5 --output-dir models/curvature_flow
"""

import argparse
import os
import sys
from pathlib import Path

import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
from torch.utils.data import DataLoader, TensorDataset
import h5py
import yaml
from tqdm import tqdm

# Try to import nflows (recommended)
try:
    from nflows import flows, transforms, distributions
    HAS_NFLOWS = False  # FORCE SimpleRealNVP for debugging
    print("nflows available but FORCING SimpleRealNVP for debugging")
except ImportError:
    HAS_NFLOWS = False
    print("Warning: nflows not installed. Will implement simple RealNVP from scratch.")

# ======================
# Simple RealNVP Implementation (if nflows not available)
# ======================

class SimpleCouplingLayer(nn.Module):
    """Simple affine coupling layer for RealNVP with ONNX-export-friendly operations."""
    
    def __init__(self, dim, hidden_dim=128, mask_type='alternate'):
        super().__init__()
        self.dim = dim
        self.hidden_dim = hidden_dim
        
        # Create binary mask: alternate dimensions
        # mask=1 means identity (unchanged), mask=0 means transform
        if mask_type == 'alternate':
            mask = torch.zeros(dim)
            mask[::2] = 1  # even indices are identity
        else:
            mask = torch.ones(dim)
            mask[::2] = 0
        self.register_buffer('mask', mask)
        
        # Number of identity and transform dimensions
        self.unchanged_dim = int(mask.sum().item())
        self.changed_dim = dim - self.unchanged_dim
        
        # Precompute permutation indices for reconstruction (ONNX-friendly)
        # identity_idx: positions in output tensor that come from identity part
        # transform_idx: positions in output tensor that come from transform part
        identity_positions = torch.where(mask == 1)[0]  # indices where identity goes
        transform_positions = torch.where(mask == 0)[0]  # indices where transform goes
        self.register_buffer('identity_positions', identity_positions)
        self.register_buffer('transform_positions', transform_positions)
        
        # Neural network for scale and shift
        # Input: identity features (unchanged part)
        # Output: scale and shift for transform part (2 * changed_dim)
        self.net = nn.Sequential(
            nn.Linear(self.unchanged_dim, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, 2 * self.changed_dim)  # scale + shift
        )
        
    def forward(self, x, reverse=False):
        # x: (batch, dim)
        # Split x into identity (x1) and transform (x2) parts using advanced indexing
        x1 = x[:, self.identity_positions]  # (batch, unchanged_dim) - identity features
        x2 = x[:, self.transform_positions]  # (batch, changed_dim) - transform features

        if self.changed_dim == 0:
            return x, torch.zeros(x.shape[0], device=x.device)

        # Network takes identity part x1 and outputs parameters for transform part x2
        net_output = self.net(x1)  # (batch, 2 * changed_dim)

        # Split into scale and shift for the transform dimensions
        scale_raw = net_output[:, :self.changed_dim]
        shift = net_output[:, self.changed_dim:2*self.changed_dim]

        # Clamp scale_raw to prevent exp overflow/underflow
        # exp(20) ≈ 4.85e8, exp(-20) ≈ 2e-9; these are safe for float32
        scale_raw = torch.clamp(scale_raw, min=-20.0, max=20.0)
        scale = torch.exp(scale_raw)

        # Apply affine transform to the transform part
        if reverse:
            # Inverse: x2 = (y2 - shift) / scale
            new_x2 = (x2 - shift) / scale
            log_det = -torch.log(scale).sum(dim=1)
        else:
            # Forward: y2 = scale * x2 + shift
            new_x2 = scale * x2 + shift
            log_det = torch.log(scale).sum(dim=1)

        # Recombine: use scatter with precomputed indices (ONNX-friendly)
        # Create empty output and scatter identity and transform parts into correct positions
        y = torch.zeros_like(x)
        # Scatter identity part
        y[:, self.identity_positions] = x1
        # Scatter transform part
        y[:, self.transform_positions] = new_x2
        return y, log_det

class StandardNormal(nn.Module):
    """Standard multivariate normal distribution without validation guards.
    
    This custom implementation avoids the data-dependent shape guards in
    torch.distributions.MultivariateNormal.log_prob() that break ONNX export.
    """
    def __init__(self, dim):
        super().__init__()
        self.dim = dim
        # Register buffers so they're part of the module state
        self.register_buffer('loc', torch.zeros(dim))
        self.register_buffer('scale', torch.ones(dim))
    
    def log_prob(self, z):
        """Compute log p(z) for standard normal.
        
        log p(z) = -0.5 * (dim * log(2π) + ||z||²)
        """
        # Sum of squares along feature dimension
        sq_norm = (z ** 2).sum(dim=1)
        log_prob = -0.5 * (self.dim * np.log(2 * np.pi) + sq_norm)
        return log_prob
    
    def sample(self, shape):
        """Sample from standard normal."""
        return torch.randn(shape, device=self.loc.device)


class SimpleRealNVP(nn.Module):
    """Stack of coupling layers to form a RealNVP flow."""
    
    def __init__(self, dim, n_layers=4, hidden_dim=128):
        super().__init__()
        self.dim = dim
        self.n_layers = n_layers
        
        layers = []
        for i in range(n_layers):
            # Alternate mask pattern
            mask_type = 'alternate' if i % 2 == 0 else 'reverse'
            layers.append(SimpleCouplingLayer(dim, hidden_dim, mask_type))
        self.layers = nn.ModuleList(layers)
        
        # Base distribution (standard Gaussian) - custom implementation for ONNX export
        self.base_dist = StandardNormal(dim)
        
    def forward(self, x, reverse=False):
        log_det_total = torch.zeros(x.shape[0], device=x.device)
        y = x
        
        if not reverse:
            # Forward: data -> latent
            for layer in self.layers:
                y, log_det = layer(y, reverse=False)
                log_det_total += log_det
        else:
            # Inverse: latent -> data
            for layer in reversed(self.layers):
                y, log_det = layer(y, reverse=True)
                log_det_total += log_det
                
        return y, log_det_total
    
    def log_prob(self, x):
        z, log_det = self.forward(x, reverse=False)
        log_prob_base = self.base_dist.log_prob(z)
        return log_prob_base + log_det
    
    def sample(self, n):
        z = torch.randn((n, self.dim), device=self.layers[0].mask.device)
        x, _ = self.forward(z, reverse=True)
        return x

# ======================
# Training
# ======================

def load_dataset(dataset_path):
    """Load curvature dataset from HDF5."""
    print(f"Loading dataset from {dataset_path}...")
    with h5py.File(dataset_path, 'r') as f:
        features = f['features'][:]
        labels = f['labels'][:]
        params = f['params'][:]
    
    print(f"Dataset: {len(features)} samples, {features.shape[1]} features")
    print(f"Label distribution:")
    unique, counts = np.unique(labels, return_counts=True)
    for label, count in zip(unique, counts):
        print(f"  Label {label}: {count} samples")
    
    return features, labels, params

def create_model(feature_dim, model_type='realnvp', n_layers=4, hidden_dim=128):
    """Create normalizing flow model."""
    if model_type.lower() == 'realnvp':
        if HAS_NFLOWS:
            # Use nflows library
            # nflows expects transform_net_create_fn(num_identity, num_transform_output)
            transform_list = []
            for i in range(n_layers):
                # Alternating mask
                mask = torch.zeros(feature_dim)
                if i % 2 == 0:
                    mask[::2] = 1.0
                else:
                    mask[1::2] = 1.0
                
                # Factory with correct signature: returns a module that accepts (identity, context)
                def make_transform_net(num_identity, num_transform_output):
                    """Create a transform net that concatenates identity and context before MLP."""
                    class TransformNet(nn.Module):
                        def __init__(self, identity_dim, output_dim):
                            super().__init__()
                            self.net = nn.Sequential(
                                nn.Linear(identity_dim, hidden_dim),
                                nn.ReLU(),
                                nn.Linear(hidden_dim, hidden_dim),
                                nn.ReLU(),
                                nn.Linear(hidden_dim, output_dim)
                            )
                        def forward(self, identity, context):
                            # For unconditional flow, context is empty; just use identity
                            return self.net(identity)
                    return TransformNet(num_identity, num_transform_output)
                
                transform_list.append(
                    transforms.AffineCouplingTransform(
                        mask=mask,
                        transform_net_create_fn=make_transform_net
                    )
                )
            transform = transforms.CompositeTransform(transform_list)
            flow = flows.Flow(transform, distributions.StandardNormal([feature_dim]))
        else:
            print("Using simple RealNVP implementation (nflows not available)")
            flow = SimpleRealNVP(feature_dim, n_layers=n_layers, hidden_dim=hidden_dim)
    else:
        raise ValueError(f"Unsupported model type: {model_type}")
    return flow

# ======================
# Training Function
# ======================

def train_flow(model, train_loader, val_loader, n_epochs=200, lr=1e-3,
               device='cpu', checkpoint_dir='models/curvature_flow'):
    """Train normalizing flow model."""
    os.makedirs(checkpoint_dir, exist_ok=True)

    model = model.to(device)
    optimizer = optim.Adam(model.parameters(), lr=lr)
    scheduler = optim.lr_scheduler.ReduceLROnPlateau(optimizer, 'min', patience=10, factor=0.5)

    train_losses = []
    val_losses = []
    best_val_loss = float('inf')
    best_epoch = 0

    print(f"Training on {device}")
    print(f"Train set: {len(train_loader.dataset)} samples")
    print(f"Val set: {len(val_loader.dataset)} samples")
    print(f"Epochs: {n_epochs}, Batch size: {train_loader.batch_size}")
    print("-" * 60)

    for epoch in range(n_epochs):
        # Training
        model.train()
        train_loss = 0.0
        n_batches = 0

        pbar = tqdm(train_loader, desc=f"Epoch {epoch+1}/{n_epochs} [train]")
        for batch_x in pbar:
            batch_x = batch_x[0].to(device)  # TensorDataset returns tuple
            optimizer.zero_grad()

            # Compute log probability
            log_prob = model.log_prob(batch_x)
            loss = -log_prob.mean()  # NLL

            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
            optimizer.step()

            train_loss += loss.item()
            n_batches += 1
            pbar.set_postfix({'loss': '{:.4f}'.format(loss.item())})

        avg_train_loss = train_loss / n_batches
        train_losses.append(avg_train_loss)

        # Validation
        model.eval()
        val_loss = 0.0
        n_val_batches = 0

        with torch.no_grad():
            for batch_x in tqdm(val_loader, desc=f"Epoch {epoch+1}/{n_epochs} [val]"):
                batch_x = batch_x[0].to(device)
                log_prob = model.log_prob(batch_x)
                loss = -log_prob.mean()
                val_loss += loss.item()
                n_val_batches += 1

        avg_val_loss = val_loss / n_val_batches
        val_losses.append(avg_val_loss)
        scheduler.step(avg_val_loss)

        # Checkpoint
        if avg_val_loss < best_val_loss:
            best_val_loss = avg_val_loss
            best_epoch = epoch
            torch.save({
                'epoch': epoch,
                'model_state_dict': model.state_dict(),
                'optimizer_state_dict': optimizer.state_dict(),
                'train_loss': avg_train_loss,
                'val_loss': avg_val_loss,
            }, os.path.join(checkpoint_dir, 'best_model.pth'))

        # Periodic save
        if (epoch + 1) % 10 == 0:
            torch.save({
                'epoch': epoch,
                'model_state_dict': model.state_dict(),
                'train_loss': avg_train_loss,
                'val_loss': avg_val_loss,
            }, os.path.join(checkpoint_dir, f'checkpoint_epoch_{epoch+1}.pth'))

        print(f"Epoch {epoch+1:3d}: train_loss={avg_train_loss:.4f}, val_loss={avg_val_loss:.4f}, "
              f"best_val={best_val_loss:.4f} (epoch {best_epoch+1})")

    # Save final model
    torch.save({
        'epoch': n_epochs,
        'model_state_dict': model.state_dict(),
        'train_loss': avg_train_loss,
        'val_loss': avg_val_loss,
    }, os.path.join(checkpoint_dir, 'final_model.pth'))

    # Save training history
    np.savez(os.path.join(checkpoint_dir, 'training_history.npz'),
             train_losses=np.array(train_losses),
             val_losses=np.array(val_losses),
             best_epoch=best_epoch,
             best_val_loss=best_val_loss)

    print(f"\nTraining complete. Best validation loss: {best_val_loss:.4f} at epoch {best_epoch+1}")
    return model, train_losses, val_losses

def main():
    parser = argparse.ArgumentParser(description='Train normalizing flow for anomaly detection')
    parser.add_argument('--dataset', type=str, default='data/curvature_dataset.h5',
                        help='Path to curvature dataset HDF5')
    parser.add_argument('--output-dir', type=str, default='models/curvature_flow',
                        help='Output directory for model checkpoints')
    parser.add_argument('--model-type', type=str, default='realnvp', choices=['realnvp', 'maf'],
                        help='Flow architecture')
    parser.add_argument('--n-layers', type=int, default=4, help='Number of coupling layers')
    parser.add_argument('--hidden-dim', type=int, default=128, help='Hidden dimension in coupling nets')
    parser.add_argument('--epochs', type=int, default=200, help='Training epochs')
    parser.add_argument('--batch-size', type=int, default=64, help='Batch size')
    parser.add_argument('--lr', type=float, default=1e-3, help='Learning rate')
    parser.add_argument('--val-split', type=float, default=0.2, help='Validation split')
    parser.add_argument('--seed', type=int, default=42, help='Random seed')
    parser.add_argument('--device', type=str, default='cuda' if torch.cuda.is_available() else 'cpu',
                        help='Device (cuda or cpu)')
    args = parser.parse_args()

    # Set seed
    torch.manual_seed(args.seed)
    np.random.seed(args.seed)

    # Load dataset
    features, labels, params = load_dataset(args.dataset)
    feature_dim = features.shape[1]

    # Normalize features (standard scaling)
    mean = features.mean(axis=0, keepdims=True)
    std = features.std(axis=0, keepdims=True) + 1e-8
    features_norm = (features - mean) / std

    # Save normalization stats
    os.makedirs(args.output_dir, exist_ok=True)
    np.savez(os.path.join(args.output_dir, 'normalization_stats.npz'),
             mean=mean, std=std)

    # Split train/val
    n_val = int(len(features_norm) * args.val_split)
    indices = np.random.permutation(len(features_norm))
    train_indices = indices[n_val:]
    val_indices = indices[:n_val]

    train_features = torch.tensor(features_norm[train_indices], dtype=torch.float32)
    val_features = torch.tensor(features_norm[val_indices], dtype=torch.float32)

    train_dataset = TensorDataset(train_features)
    val_dataset = TensorDataset(val_features)

    train_loader = DataLoader(train_dataset, batch_size=args.batch_size, shuffle=True)
    val_loader = DataLoader(val_dataset, batch_size=args.batch_size, shuffle=False)

    print(f"\nTrain set: {len(train_features)} samples")
    print(f"Val set: {len(val_features)} samples")
    print(f"Feature dimension: {feature_dim}")
    print(f"Device: {args.device}")
    print(f"Model: {args.model_type} with {args.n_layers} layers, hidden_dim={args.hidden_dim}")

    # Create model
    model = create_model(feature_dim, model_type=args.model_type,
                         n_layers=args.n_layers, hidden_dim=args.hidden_dim)

    # Count parameters
    n_params = sum(p.numel() for p in model.parameters())
    print(f"Model parameters: {n_params:,}")

    # Train
    model, train_losses, val_losses = train_flow(
        model, train_loader, val_loader,
        n_epochs=args.epochs, lr=args.lr,
        device=args.device, checkpoint_dir=args.output_dir
    )

    # Save final normalization stats in model metadata
    metadata = {
        'feature_dim': feature_dim,
        'model_type': args.model_type,
        'n_layers': args.n_layers,
        'hidden_dim': args.hidden_dim,
        'train_samples': len(train_features),
        'val_samples': len(val_features),
        'mean': mean.tolist(),
        'std': std.tolist(),
    }
    with open(os.path.join(args.output_dir, 'metadata.yaml'), 'w') as f:
        yaml.dump(metadata, f)

    print(f"\nModel saved to {args.output_dir}")
    print(f"  - best_model.pth (best checkpoint)")
    print(f"  - final_model.pth (final epoch)")
    print(f"  - normalization_stats.npz (feature scaling)")
    print(f"  - metadata.yaml (model config)")
    print(f"  - training_history.npz (loss curves)")

if __name__ == '__main__':
    main()
