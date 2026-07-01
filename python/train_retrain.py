#!/usr/bin/env python3
"""
QuantumVerse - Task 3.3: Lightweight Retrain for Geodesic Neural ODE Surrogate
Memory-efficient training with incremental stats and reduced dataset.
"""

import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader
import numpy as np
import h5py
import json
from pathlib import Path
from typing import Tuple, Dict, Optional
import argparse
import time
from datetime import datetime
import os

os.environ['PYTHONIOENCODING'] = 'utf-8'

device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
print(f"Using device: {device}")


def compute_stats_incremental(h5_path: str, max_samples: int = 100000):
    """Compute mean and std incrementally using Welford's algorithm."""
    with h5py.File(h5_path, 'r') as f:
        N = min(len(f['final_event']), max_samples)
        n_features = 10  # 4 event + 4 vel + 1 tau + 1 metric_param
        n_output = 4

        # Welford's online algorithm
        count = 0
        mean_in = np.zeros(n_features, dtype=np.float64)
        M2_in = np.zeros(n_features, dtype=np.float64)
        mean_out = np.zeros(n_output, dtype=np.float64)
        M2_out = np.zeros(n_output, dtype=np.float64)

        chunk_size = 10000
        for i in range(0, N, chunk_size):
            end = min(i + chunk_size, N)
            n = end - i

            initial_events = f['initial_event'][i:end]
            initial_velocities = f['initial_velocity'][i:end]
            taus = f['tau'][i:end].reshape(-1, 1)
            metric_params = f['metric_params'][i:end].reshape(-1, 1)
            final_events = f['final_event'][i:end]

            inputs = np.concatenate([
                initial_events, initial_velocities, taus, metric_params
            ], axis=1).astype(np.float64)
            outputs = final_events.astype(np.float64)

            # Update running stats for inputs
            for j in range(n):
                count += 1
                delta = inputs[j] - mean_in
                mean_in += delta / count
                delta2 = inputs[j] - mean_in
                M2_in += delta * delta2

                delta_o = outputs[j] - mean_out
                mean_out += delta_o / count
                delta2_o = outputs[j] - mean_out
                M2_out += delta_o * delta2_o

    std_in = np.sqrt(M2_in / count) if count > 1 else np.ones(n_features)
    std_out = np.sqrt(M2_out / count) if count > 1 else np.ones(n_output)

    # Avoid division by zero
    std_in[std_in < 1e-8] = 1.0
    std_out[std_out < 1e-8] = 1.0

    return mean_in.astype(np.float32), std_in.astype(np.float32), \
           mean_out.astype(np.float32), std_out.astype(np.float32)


class GeodesicDataset(Dataset):
    """Memory-efficient dataset that loads samples on-the-fly."""

    def __init__(self, h5_path: str, split: str = 'train',
                 train_frac: float = 0.8, max_samples: int = 100000,
                 input_mean: np.ndarray = None, input_std: np.ndarray = None,
                 output_mean: np.ndarray = None, output_std: np.ndarray = None):
        self.h5_path = h5_path
        self.split = split
        self.input_mean = input_mean
        self.input_std = input_std
        self.output_mean = output_mean
        self.output_std = output_std
        self.h5_file = None

        with h5py.File(h5_path, 'r') as f:
            total = len(f['final_event'])
            use_n = min(total, max_samples)
            indices = np.arange(use_n)
            np.random.seed(42)
            np.random.shuffle(indices)

            n_train = int(use_n * train_frac)
            n_val = (use_n - n_train) // 2

            if split == 'train':
                self.indices = indices[:n_train]
            elif split == 'val':
                self.indices = indices[n_train:n_train + n_val]
            else:
                self.indices = indices[n_train + n_val:]

            self.N = len(self.indices)

        print(f"  {split}: {self.N} samples")

    def __len__(self):
        return self.N

    def __getitem__(self, idx: int) -> Tuple[torch.Tensor, torch.Tensor]:
        if self.h5_file is None:
            self.h5_file = h5py.File(self.h5_path, 'r')

        i = self.indices[idx]
        initial_event = self.h5_file['initial_event'][i]
        initial_velocity = self.h5_file['initial_velocity'][i]
        tau = self.h5_file['tau'][i]
        metric_params = self.h5_file['metric_params'][i]
        final_event = self.h5_file['final_event'][i]

        input_vec = np.concatenate([
            initial_event.ravel(), initial_velocity.ravel(),
            [float(tau)], [float(metric_params)]
        ]).astype(np.float32)

        # Normalize
        input_norm = (input_vec - self.input_mean) / self.input_std
        output_norm = (final_event.astype(np.float32) - self.output_mean) / self.output_std

        return torch.tensor(input_norm, dtype=torch.float32), \
               torch.tensor(output_norm, dtype=torch.float32)


class GeodesicMLP(nn.Module):
    """Compact MLP for geodesic prediction."""

    def __init__(self, input_dim: int = 10, hidden_dim: int = 128,
                 output_dim: int = 4, num_layers: int = 2):
        super().__init__()
        layers = []
        in_dim = input_dim
        for _ in range(num_layers):
            layers.append(nn.Linear(in_dim, hidden_dim))
            layers.append(nn.ReLU())
            in_dim = hidden_dim
        layers.append(nn.Linear(in_dim, output_dim))
        self.network = nn.Sequential(*layers)
        self.apply(self._init_weights)

    def _init_weights(self, m):
        if isinstance(m, nn.Linear):
            nn.init.kaiming_normal_(m.weight, mode='fan_in', nonlinearity='relu')
            if m.bias is not None:
                nn.init.zeros_(m.bias)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.network(x)


def train_epoch(model, dataloader, optimizer, criterion, device):
    model.train()
    total_loss = 0.0
    for inputs, targets in dataloader:
        inputs, targets = inputs.to(device), targets.to(device)
        optimizer.zero_grad()
        predictions = model(inputs)
        loss = criterion(predictions, targets)
        loss.backward()
        torch.nn.utils.clip_grad_norm_(model.parameters(), max_norm=1.0)
        optimizer.step()
        total_loss += loss.item()
    return total_loss / len(dataloader)


def validate(model, dataloader, criterion, device):
    model.eval()
    total_loss = 0.0
    with torch.no_grad():
        for inputs, targets in dataloader:
            inputs, targets = inputs.to(device), targets.to(device)
            predictions = model(inputs)
            loss = criterion(predictions, targets)
            total_loss += loss.item()
    return total_loss / len(dataloader)


def export_to_onnx(model, sample_input, onnx_path, input_mean, input_std,
                   output_mean, output_std):
    """Export model with baked-in normalization."""
    model.eval()
    sample_input = sample_input.cpu()

    class NormalizedModel(nn.Module):
        def __init__(self, base_model, input_mean, input_std, output_mean, output_std):
            super().__init__()
            self.base_model = base_model
            self.register_buffer('input_mean', torch.tensor(input_mean, dtype=torch.float32))
            self.register_buffer('input_std', torch.tensor(input_std, dtype=torch.float32))
            self.register_buffer('output_mean', torch.tensor(output_mean, dtype=torch.float32))
            self.register_buffer('output_std', torch.tensor(output_std, dtype=torch.float32))

        def forward(self, x):
            x = (x - self.input_mean) / self.input_std
            out = self.base_model(x)
            return out * self.output_std + self.output_mean

    wrapped = NormalizedModel(model, input_mean, input_std, output_mean, output_std)
    wrapped.eval()

    import sys, io
    old_stdout = sys.stdout
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
    try:
        torch.onnx.export(
            wrapped, sample_input, str(onnx_path),
            export_params=True, opset_version=14,
            do_constant_folding=True,
            input_names=['input'], output_names=['final_event'],
            dynamic_axes={'input': {0: 'batch_size'}, 'final_event': {0: 'batch_size'}}
        )
    finally:
        sys.stdout = old_stdout
    print(f"Model exported to ONNX: {onnx_path}")


def main():
    parser = argparse.ArgumentParser(description="Train Geodesic Neural ODE (lightweight)")
    parser.add_argument('--dataset', type=str, default='data/geodesics_1M.h5')
    parser.add_argument('--output-dir', type=str, default='models/geodesic_ode')
    parser.add_argument('--epochs', type=int, default=80)
    parser.add_argument('--batch-size', type=int, default=256)
    parser.add_argument('--lr', type=float, default=5e-4)
    parser.add_argument('--max-samples', type=int, default=100000,
                        help='Max samples to use from dataset')
    args = parser.parse_args()

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    hparams = {
        'hidden_dim': 128,
        'num_layers': 2,
        'learning_rate': args.lr,
        'batch_size': args.batch_size,
        'num_epochs': args.epochs,
        'weight_decay': 1e-5,
        'max_samples': args.max_samples,
    }

    with open(output_dir / 'hparams.json', 'w') as f:
        json.dump(hparams, f, indent=2)

    print("Computing normalization stats (incremental)...")
    input_mean, input_std, output_mean, output_std = compute_stats_incremental(
        args.dataset, max_samples=args.max_samples)
    print(f"  Input mean: {input_mean}")
    print(f"  Input std:  {input_std}")

    norm_stats = {
        'mean': input_mean.tolist(),
        'std': input_std.tolist(),
        'output_mean': output_mean.tolist(),
        'output_std': output_std.tolist(),
        'input_dim': len(input_mean)
    }
    with open(output_dir / 'normalization.json', 'w') as f:
        json.dump(norm_stats, f, indent=2)

    print("Loading datasets...")
    train_dataset = GeodesicDataset(args.dataset, split='train',
                                    max_samples=args.max_samples,
                                    input_mean=input_mean, input_std=input_std,
                                    output_mean=output_mean, output_std=output_std)
    val_dataset = GeodesicDataset(args.dataset, split='val',
                                  max_samples=args.max_samples,
                                  input_mean=input_mean, input_std=input_std,
                                  output_mean=output_mean, output_std=output_std)

    train_loader = DataLoader(train_dataset, batch_size=hparams['batch_size'],
                              shuffle=True, num_workers=0)
    val_loader = DataLoader(val_dataset, batch_size=hparams['batch_size'],
                            shuffle=False, num_workers=0)

    sample_input, _ = train_dataset[0]
    input_dim = sample_input.shape[0]
    output_dim = 4
    print(f"Input dim: {input_dim}, Output dim: {output_dim}")

    model = GeodesicMLP(
        input_dim=input_dim,
        hidden_dim=hparams['hidden_dim'],
        output_dim=output_dim,
        num_layers=hparams['num_layers']
    ).to(device)

    total_params = sum(p.numel() for p in model.parameters())
    print(f"Model parameters: {total_params:,}")

    criterion = nn.MSELoss()
    optimizer = optim.Adam(model.parameters(), lr=hparams['learning_rate'],
                           weight_decay=hparams['weight_decay'])
    scheduler = optim.lr_scheduler.ReduceLROnPlateau(optimizer, mode='min',
                                                      factor=0.5, patience=5)

    print(f"\nStarting training ({args.epochs} epochs)...")
    best_val_loss = float('inf')

    for epoch in range(hparams['num_epochs']):
        start_time = time.time()
        train_loss = train_epoch(model, train_loader, optimizer, criterion, device)
        val_loss = validate(model, val_loader, criterion, device)
        scheduler.step(val_loss)
        epoch_time = time.time() - start_time

        print(f"Epoch {epoch+1}/{hparams['num_epochs']} | "
              f"Train: {train_loss:.6f} | Val: {val_loss:.6f} | "
              f"Time: {epoch_time:.1f}s")

        if val_loss < best_val_loss:
            best_val_loss = val_loss
            checkpoint = {
                'epoch': epoch,
                'model_state_dict': model.state_dict(),
                'optimizer_state_dict': optimizer.state_dict(),
                'train_loss': train_loss,
                'val_loss': val_loss
            }
            torch.save(checkpoint, output_dir / 'best_model.pth')
            print(f"  -> New best model (val_loss={val_loss:.6f})")

    print("\nTraining complete. Exporting to ONNX...")
    best_checkpoint = torch.load(output_dir / 'best_model.pth', map_location=device)
    model.load_state_dict(best_checkpoint['model_state_dict'])

    sample_batch = next(iter(val_loader))[0][:1].to(device)
    onnx_path = output_dir / 'geodesic_ode.onnx'
    export_to_onnx(model, sample_batch, onnx_path,
                   input_mean, input_std, output_mean, output_std)

    metadata = {
        'train_loss': float(best_checkpoint['train_loss']),
        'val_error': float(best_checkpoint['val_loss']),
        'num_epochs': hparams['num_epochs'],
        'input_dim': int(input_dim),
        'output_dim': output_dim,
        'hidden_dim': hparams['hidden_dim'],
        'num_layers': hparams['num_layers'],
        'date_trained': datetime.now().isoformat(),
        'dataset': args.dataset,
        'max_samples_used': args.max_samples,
        'input_normalization': 'zero_mean_unit_variance',
        'output_normalization': 'zero_mean_unit_variance'
    }
    with open(output_dir / 'metadata.json', 'w') as f:
        json.dump(metadata, f, indent=2)

    print(f"\nModel and metadata saved to {output_dir}")
    print(f"Final val_loss (MSE): {best_val_loss:.6f}")
    print(f"Final val_loss (RMSE per component): {best_val_loss**0.5:.6f}")
    return str(onnx_path)


if __name__ == '__main__':
    main()