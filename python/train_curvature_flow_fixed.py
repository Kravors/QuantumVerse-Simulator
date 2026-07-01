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
    HAS_NFLOWS = True
except ImportError:
    HAS_NFLOWS = False
    print("Warning: nflows not installed. Will implement simple RealNVP from scratch.")

# ======================
# Simple RealNVP Implementation (if nflows not available)
# ======================

class SimpleCouplingLayer(nn.Module):
    """Simple affine coupling layer for RealNVP."""
    
    def __init__(self, dim, hidden_dim=128, mask_type='alternate'):
        super().__init__()
        self.dim = dim
        self.hidden_dim = hidden_dim
        
        # Create binary mask: alternate dimensions
        if mask_type == 'alternate':
            mask = torch.zeros(dim)
            mask[::2] = 1  # even indices are identity
        else:
            mask = torch.ones(dim)
            mask[::2] = 0
        self.register_buffer('mask', mask)
        
        # Neural network for scale and shift
        self.net = nn.Sequential(
            nn.Linear(int(dim * (1 - mask.sum().item())), hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, dim)  # outputs both scale and shift (2*dim actually)
        )
        
    def forward(self, x, reverse=False):
        # x: (batch, dim)
        masked_x = x * self.mask
        # Get the unchanged part size
        unchanged_dim = int(self.mask.sum().item())
        changed_dim = self.dim - unchanged_dim
        
        if changed_dim == 0:
            # All dimensions masked (shouldn't happen)
            return x, torch.zeros(x.shape[0], device=x.device)
        
        # Input to network: only the masked (unchanged) dimensions
        net_input = x * (1 - self.mask)  # the parts that will be transformed
        net_output = self.net(net_input)
        
        # Split into scale and shift
        scale = torch.exp(net_output[:, :changed_dim])  # ensure scale > 0
        shift = net_output[:, changed_dim:changed_dim*2] if changed_dim*2 <= net_output.shape[1] else net_output[:, changed_dim:]
        
        # Apply transform
        if reverse:
            # Inverse: x' = (x - shift) / scale
            y = masked_x + (1 - self.mask) * (x - shift) / scale
            log_det = -torch.log(scale).sum(dim=1)
        else:
            # Forward: y = scale * x + shift
            y = masked_x + (1 - self.mask) * (scale * (1 - self.mask) * x + shift)
            log_det = torch.log(scale).sum(dim=1)
        
        return y, log_det

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
        
        # Base distribution (standard Gaussian)
        self.base_dist = torch.distributions.MultivariateNormal(
            torch.zeros(dim), torch.eye(dim)
        )
        
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
        z = self.base_dist.sample((n,))
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
                
                # Factory with correct signature
                def make_transform_net(num_identity, num_transform_output):
                    return nn.Sequential(
                        nn.Linear(num_identity, hidden_dim),
                        nn.ReLU(),
                        nn.Linear(hidden_dim, hidden_dim),
                        nn.ReLU(),
                        nn.Linear(hidden_dim, num_transform_output)
                    )
                
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
if __name__ == '__main__':
    main()
