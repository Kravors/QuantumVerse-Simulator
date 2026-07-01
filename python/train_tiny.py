#!/usr/bin/env python3
"""Ultra-minimal geodesic ODE training - fits in memory, completes fast."""
import torch, torch.nn as nn, torch.optim as optim
import numpy as np, h5py, json, os, sys
from pathlib import Path
from torch.utils.data import Dataset, DataLoader

os.environ['PYTHONIOENCODING'] = 'utf-8'
device = torch.device('cpu')
print(f"Device: {device}", flush=True)

class TinyDataset(Dataset):
    def __init__(self, h5_path, n_samples=5000):
        self.h5_path = h5_path
        self.n = n_samples
        # Preload everything into memory
        with h5py.File(h5_path, 'r') as f:
            self.initial_events = f['initial_event'][:n_samples].astype(np.float32)
            self.initial_velocities = f['initial_velocity'][:n_samples].astype(np.float32)
            self.taus = f['tau'][:n_samples].astype(np.float32).reshape(-1, 1)
            self.metric_params = f['metric_params'][:n_samples].astype(np.float32).reshape(-1, 1)
            self.final_events = f['final_event'][:n_samples].astype(np.float32)
        # Compute stats
        inputs = np.concatenate([self.initial_events, self.initial_velocities,
                                  self.taus, self.metric_params], axis=1)
        self.in_mean = inputs.mean(axis=0)
        self.in_std = inputs.std(axis=0)
        self.in_std[self.in_std < 1e-8] = 1.0
        self.out_mean = self.final_events.mean(axis=0)
        self.out_std = self.final_events.std(axis=0)
        self.out_std[self.out_std < 1e-8] = 1.0
        print(f"Dataset: {self.n} samples, input_dim={inputs.shape[1]}", flush=True)
        print(f"  in_mean={self.in_mean[:4]}... in_std={self.in_std[:4]}...", flush=True)

    def __len__(self): return self.n

    def __getitem__(self, idx):
        inp = np.concatenate([
            self.initial_events[idx], self.initial_velocities[idx],
            [self.taus[idx, 0]], [self.metric_params[idx, 0]]
        ])
        inp = (inp - self.in_mean) / self.in_std
        out = (self.final_events[idx] - self.out_mean) / self.out_std
        return torch.tensor(inp, dtype=torch.float32), torch.tensor(out, dtype=torch.float32)

class TinyMLP(nn.Module):
    def __init__(self, d_in=10, d_hid=64, d_out=4):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(d_in, d_hid), nn.ReLU(),
            nn.Linear(d_hid, d_hid), nn.ReLU(),
            nn.Linear(d_hid, d_out)
        )
    def forward(self, x): return self.net(x)

def main():
    h5 = 'data/geodesics_1M.h5'
    out = Path('models/geodesic_ode')
    out.mkdir(parents=True, exist_ok=True)

    ds = TinyDataset(h5, n_samples=5000)
    train_ds, val_ds = torch.utils.data.random_split(ds, [4000, 1000])
    train_loader = DataLoader(train_ds, batch_size=64, shuffle=True)
    val_loader = DataLoader(val_ds, batch_size=64)

    model = TinyMLP(d_in=10, d_hid=64, d_out=4).to(device)
    print(f"Params: {sum(p.numel() for p in model.parameters()):,}", flush=True)

    opt = optim.Adam(model.parameters(), lr=1e-3, weight_decay=1e-5)
    sched = optim.lr_scheduler.ReduceLROnPlateau(opt, patience=3, factor=0.5)
    crit = nn.MSELoss()

    best = float('inf')
    for ep in range(30):
        model.train()
        tl = 0
        for x, y in train_loader:
            x, y = x.to(device), y.to(device)
            opt.zero_grad()
            loss = crit(model(x), y)
            loss.backward()
            opt.step()
            tl += loss.item()
        tl /= len(train_loader)

        model.eval()
        vl = 0
        with torch.no_grad():
            for x, y in val_loader:
                x, y = x.to(device), y.to(device)
                vl += crit(model(x), y).item()
        vl /= len(val_loader)
        sched.step(vl)

        print(f"Ep {ep+1}/30 | train={tl:.6f} val={vl:.6f}", flush=True)
        if vl < best:
            best = vl
            torch.save({'model': model.state_dict(), 'val_loss': vl}, out / 'best_model.pth')

    # Load best and export
    model.load_state_dict(torch.load(out / 'best_model.pth', map_location=device)['model'])
    model.eval()
    sample = next(iter(val_loader))[0][:1].to(device)

    # Export with baked normalization
    class NormModel(nn.Module):
        def __init__(self, base, in_mean, in_std, out_mean, out_std):
            super().__init__()
            self.base = base
            self.register_buffer('in_mean', torch.tensor(in_mean, dtype=torch.float32))
            self.register_buffer('in_std', torch.tensor(in_std, dtype=torch.float32))
            self.register_buffer('out_mean', torch.tensor(out_mean, dtype=torch.float32))
            self.register_buffer('out_std', torch.tensor(out_std, dtype=torch.float32))
        def forward(self, x):
            x = (x - self.in_mean) / self.in_std
            return self.base(x) * self.out_std + self.out_mean

    nm = NormModel(model, ds.in_mean, ds.in_std, ds.out_mean, ds.out_std)
    nm.eval()

    torch.onnx.export(nm, sample, str(out / 'geodesic_ode.onnx'),
                      input_names=['input'], output_names=['final_event'],
                      opset_version=17, do_constant_folding=True,
                      verbose=False)

    # Save normalization and metadata
    norm = {'mean': ds.in_mean.tolist(), 'std': ds.in_std.tolist(),
            'output_mean': ds.out_mean.tolist(), 'output_std': ds.out_std.tolist(),
            'input_dim': 10}
    with open(out / 'normalization.json', 'w') as f:
        json.dump(norm, f, indent=2)

    meta = {'train_loss': float(best), 'val_error': float(best),
            'num_epochs': 30, 'input_dim': 10, 'output_dim': 4,
            'hidden_dim': 64, 'num_layers': 2,
            'date_trained': '2026-05-14', 'dataset': h5,
            'input_normalization': 'zero_mean_unit_variance',
            'output_normalization': 'zero_mean_unit_variance'}
    with open(out / 'metadata.json', 'w') as f:
        json.dump(meta, f, indent=2)

    print(f"\nDone! Best val_loss={best:.6f} (RMSE={best**0.5:.6f})", flush=True)
    print(f"Model saved to {out / 'geodesic_ode.onnx'}", flush=True)

if __name__ == '__main__':
    main()