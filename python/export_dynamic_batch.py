"""
Re-export the geodesic ODE ONNX model with dynamic batch dimension.

The original model has a fixed batch dimension of 1, which prevents
batched inference. This script re-exports it with a dynamic batch axis
so the C++ predictBatch() function can run with any batch size.

Usage: python export_dynamic_batch.py
"""

import torch
import torch.nn as nn
import numpy as np
import json
import os
import sys

MODEL_DIR = "models/geodesic_ode"
ONNX_PATH = os.path.join(MODEL_DIR, "geodesic_ode.onnx")
DYNAMIC_ONNX_PATH = os.path.join(MODEL_DIR, "geodesic_ode_dynamic.onnx")
NORM_PATH = os.path.join(MODEL_DIR, "normalization.json")


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
    print("=" * 60)
    print("  Re-exporting ONNX model with dynamic batch dimension")
    print("=" * 60)

    # Load normalization stats
    with open(NORM_PATH) as f:
        norm = json.load(f)

    in_mean = np.array(norm['mean'], dtype=np.float32)
    in_std = np.array(norm['std'], dtype=np.float32)
    out_mean = np.array(norm['output_mean'], dtype=np.float32)
    out_std = np.array(norm['output_std'], dtype=np.float32)

    # Rebuild and load trained model
    model = TinyMLP(d_in=10, d_hid=64, d_out=4)
    ckpt = torch.load(os.path.join(MODEL_DIR, 'best_model.pth'), map_location='cpu')
    model.load_state_dict(ckpt['model'])
    model.eval()

    # Wrap with normalization
    nm = NormModel(model, in_mean, in_std, out_mean, out_std)
    nm.eval()

    # Use batch size 1 for the export sample, but mark batch as dynamic
    sample = torch.randn(1, 10, dtype=torch.float32)

    # Export with dynamic batch axis
    torch.onnx.export(
        nm,
        sample,
        DYNAMIC_ONNX_PATH,
        input_names=['input'],
        output_names=['final_event'],
        dynamic_axes={
            'input': {0: 'batch_size'},
            'final_event': {0: 'batch_size'}
        },
        opset_version=17,
        do_constant_folding=True,
    )

    # Verify
    try:
        import onnx
        onnx_model = onnx.load(DYNAMIC_ONNX_PATH)
        onnx.checker.check_model(onnx_model)

        # Confirm dynamic batch
        graph_input = onnx_model.graph.input[0]
        dim = graph_input.type.tensor_type.shape.dim[0]
        print(f"  Input shape: {graph_input.name} dim_param={dim.dim_param}")

        orig_size = os.path.getsize(ONNX_PATH)
        new_size = os.path.getsize(DYNAMIC_ONNX_PATH)
        print(f"  Original model: {orig_size / 1024:.1f} KB")
        print(f"  Dynamic model:  {new_size / 1024:.1f} KB")
        print(f"  ONNX validation: PASSED")
        print(f"\n  Dynamic model saved to: {DYNAMIC_ONNX_PATH}")
    except ImportError:
        print("  onnx not installed, skipping validation")
    except Exception as e:
        print(f"  ONNX validation warning: {e}")

    # Quick test with different batch sizes
    print("\n  Testing with various batch sizes via ONNX Runtime:")
    try:
        import onnxruntime as ort
        sess = ort.InferenceSession(DYNAMIC_ONNX_PATH)

        for bs in [1, 4, 16, 64, 256, 1024]:
            test_input = np.random.randn(bs, 10).astype(np.float32)
            output = sess.run(['final_event'], {'input': test_input})
            assert output[0].shape[0] == bs, f"Expected batch {bs}, got {output[0].shape[0]}"
            print(f"    batch={bs:5d}: output shape={output[0].shape} OK")

        print("  All batch sizes work!")
    except Exception as e:
        print(f"  Error: {e}")

    print("Done!")
    return 0


if __name__ == "__main__":
    sys.exit(main())