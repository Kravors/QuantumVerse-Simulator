#!/usr/bin/env python3
"""Export the trained geodesic ODE model to ONNX with encoding fix."""
import torch, torch.nn as nn
import numpy as np, h5py, json, os, sys, io
from pathlib import Path

os.environ['PYTHONIOENCODING'] = 'utf-8'

class TinyMLP(nn.Module):
    def __init__(self, d_in=10, d_hid=64, d_out=4):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(d_in, d_hid), nn.ReLU(),
            nn.Linear(d_hid, d_hid), nn.ReLU(),
            nn.Linear(d_hid, d_out)
        )
    def forward(self, x):
        return self.net(x)

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

def main():
    out = Path('models/geodesic_ode')

    # Load normalization stats
    with open(out / 'normalization.json', 'r') as f:
        norm = json.load(f)
    in_mean = np.array(norm['mean'], dtype=np.float32)
    in_std = np.array(norm['std'], dtype=np.float32)
    out_mean = np.array(norm['output_mean'], dtype=np.float32)
    out_std = np.array(norm['output_std'], dtype=np.float32)

    # Rebuild and load trained model
    model = TinyMLP(d_in=10, d_hid=64, d_out=4)
    ckpt = torch.load(out / 'best_model.pth', map_location='cpu')
    model.load_state_dict(ckpt['model'])
    model.eval()
    print(f"Loaded model with val_loss={ckpt['val_loss']:.6f}")

    # Create sample input
    with h5py.File('data/geodesics_1M.h5', 'r') as f:
        n = min(100, len(f['final_event']))
        initial_events = f['initial_event'][:n].astype(np.float32)
        initial_velocities = f['initial_velocity'][:n].astype(np.float32)
        taus = f['tau'][:n].astype(np.float32).reshape(-1, 1)
        metric_params = f['metric_params'][:n].astype(np.float32).reshape(-1, 1)
        final_events = f['final_event'][:n].astype(np.float32)

    inputs = np.concatenate([initial_events, initial_velocities, taus, metric_params], axis=1)
    inputs_norm = (inputs - in_mean) / in_std
    sample = torch.tensor(inputs_norm[:1], dtype=torch.float32)

    # Wrap with normalization baked in
    nm = NormModel(model, in_mean, in_std, out_mean, out_std)
    nm.eval()

    # Quick validation
    with torch.no_grad():
        pred_norm = model(sample)
        pred = pred_norm * torch.tensor(out_std, dtype=torch.float32) + torch.tensor(out_mean, dtype=torch.float32)
        true = torch.tensor(final_events[:1], dtype=torch.float32)
        err = torch.abs(pred - true)
        print(f"Sample prediction error: {err.mean().item():.6f}")

    # Export to ONNX - suppress stdout to avoid encoding issues
    old_stdout = sys.stdout
    old_stderr = sys.stderr
    sys.stdout = io.TextIOWrapper(io.BytesIO(), encoding='utf-8')
    sys.stderr = io.TextIOWrapper(io.BytesIO(), encoding='utf-8')
    try:
        torch.onnx.export(
            nm, sample, str(out / 'geodesic_ode.onnx'),
            input_names=['input'], output_names=['final_event'],
            opset_version=17, do_constant_folding=True
        )
    except Exception as e:
        sys.stdout = old_stdout
        sys.stderr = old_stderr
        print(f"ONNX export failed: {e}")
        raise
    finally:
        sys.stdout = old_stdout
        sys.stderr = old_stderr

    print(f"Model exported to {out / 'geodesic_ode.onnx'}")

    # Verify ONNX model
    try:
        import onnx
        onnx_model = onnx.load(str(out / 'geodesic_ode.onnx'))
        onnx.checker.check_model(onnx_model)
        print("ONNX model validation: PASSED")
    except ImportError:
        print("onnx not installed, skipping validation")
    except Exception as e:
        print(f"ONNX validation warning: {e}")

    # Update metadata
    with open(out / 'metadata.json', 'r') as f:
        meta = json.load(f)
    meta['onnx_opset'] = 17
    meta['export_date'] = '2026-05-14'
    with open(out / 'metadata.json', 'w') as f:
        json.dump(meta, f, indent=2)

    print("Done!")

if __name__ == '__main__':
    main()