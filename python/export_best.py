#!/usr/bin/env python3
"""
Export the current best model to ONNX for C++ inference.
This allows us to proceed with integration while training continues.
"""

import torch
import h5py
import json
from pathlib import Path
from torch.utils.data import Dataset, DataLoader

class GeodesicDataset(Dataset):
    def __init__(self, h5_path: Path, split: str = 'val'):
        super().__init__()
        self.h5_path = h5_path
        with h5py.File(h5_path, 'r') as f:
            N = len(f['final_event'])
            # Use same split as training: 80% train, 10% val, 10% test
            n_train = int(N * 0.8)
            n_val = (N - n_train) // 2
            if split == 'train':
                self.indices = list(range(n_train))
            elif split == 'val':
                self.indices = list(range(n_train, n_train+n_val))
            else:
                self.indices = list(range(n_train+n_val, N))
            self.N = len(self.indices)
            self.h5_file = None

    def __len__(self):
        return self.N

    def __getitem__(self, idx):
        if self.h5_file is None:
            self.h5_file = h5py.File(self.h5_path, 'r')
        i = self.indices[idx]
        initial_event = self.h5_file['initial_event'][i]
        initial_velocity = self.h5_file['initial_velocity'][i]
        metric_params = self.h5_file['metric_params'][i]
        tau = self.h5_file['tau'][i]
        final_event = self.h5_file['final_event'][i]
        input_vec = torch.cat([torch.tensor(initial_event, dtype=torch.float32),
                               torch.tensor(initial_velocity, dtype=torch.float32),
                               torch.tensor([tau], dtype=torch.float32),
                               torch.tensor(metric_params, dtype=torch.float32)])
        return input_vec, torch.tensor(final_event, dtype=torch.float32)

class GeodesicMLP(torch.nn.Module):
    def __init__(self, input_dim, hidden_dim, output_dim, num_layers):
        super().__init__()
        layers = []
        in_dim = input_dim
        for _ in range(num_layers):
            layers.append(torch.nn.Linear(in_dim, hidden_dim))
            layers.append(torch.nn.ReLU())
            layers.append(torch.nn.BatchNorm1d(hidden_dim))
            in_dim = hidden_dim
        layers.append(torch.nn.Linear(in_dim, output_dim))
        self.network = torch.nn.Sequential(*layers)
    def forward(self, x):
        return self.network(x)

def main():
    model_dir = Path('models/geodesic_ode')
    dataset_path = Path('data/geodesics_1M.h5')

    # Load hparams
    with open(model_dir / 'hparams.json') as f:
        hparams = json.load(f)

    # Load best checkpoint
    device = torch.device('cpu')
    checkpoint = torch.load(model_dir / 'best_model.pth', map_location=device)
    input_dim = checkpoint['model_state_dict']['network.0.weight'].shape[1]
    output_dim = 4

    # Create model and load weights
    model = GeodesicMLP(input_dim=input_dim,
                        hidden_dim=hparams['hidden_dim'],
                        output_dim=output_dim,
                        num_layers=hparams['num_layers'])
    model.load_state_dict(checkpoint['model_state_dict'])
    model.eval()

    # Get a sample from validation set
    val_dataset = GeodesicDataset(dataset_path, split='val')
    sample_input, _ = val_dataset[0]
    sample_input = sample_input.unsqueeze(0)  # add batch dim

    # Export to ONNX
    onnx_path = model_dir / 'geodesic_ode.onnx'
    torch.onnx.export(
        model,
        sample_input,
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
    print(f"ONNX model exported to {onnx_path}")

    # Save updated metadata
    metadata = {
        'train_loss': float(checkpoint['train_loss']),
        'val_error': float(checkpoint['val_loss']),
        'num_epochs': hparams['num_epochs'],
        'input_dim': int(input_dim),
        'output_dim': int(output_dim),
        'hidden_dim': hparams['hidden_dim'],
        'num_layers': hparams['num_layers'],
        'date_trained': str(torch.load(model_dir / 'checkpoint_epoch95.pth')['epoch']+1) if (model_dir/'checkpoint_epoch95.pth').exists() else '95',
        'dataset': str(dataset_path),
        'note': 'Exported from best model (training may still be in progress)'
    }
    with open(model_dir / 'metadata.json', 'w') as f:
        json.dump(metadata, f, indent=2)
    print("Metadata saved.")

if __name__ == '__main__':
    main()
