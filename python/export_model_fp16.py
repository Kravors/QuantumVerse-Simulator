"""
export_model_fp16.py
=====================
Convert a GeodesicNeuralODE ONNX model from FP32 to FP16 (half precision).

FP16 cuts model size in half and typically gives a 1.5–2× speedup on NVIDIA
GPUs with Tensor Cores (compute capability ≥ 7.0, e.g. GTX 1650 with CC 7.5).

Usage:
    python export_model_fp16.py  [--input  models/geodesic_ode/geodesic_ode.onnx]
                                 [--output models/geodesic_ode/geodesic_ode_fp16.onnx]
                                 [--force]

The script writes a new file alongside the original by default.  Pass --force
to overwrite an existing FP16 file.
"""

import argparse
import os
import sys

try:
    import onnx
    from onnxconverter_common import float16
except ImportError:
    print(
        "[ERROR] Required packages not found.\n"
        "  Install with: pip install onnx onnxconverter-common\n"
    )
    sys.exit(1)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert GeodesicNeuralODE ONNX model to FP16"
    )
    default_input = os.path.join("models", "geodesic_ode", "geodesic_ode.onnx")
    default_output = os.path.join("models", "geodesic_ode", "geodesic_ode_fp16.onnx")
    parser.add_argument(
        "--input", "-i", default=default_input,
        help=f"Path to input FP32 ONNX model (default: {default_input})"
    )
    parser.add_argument(
        "--output", "-o", default=default_output,
        help=f"Path to output FP16 ONNX model (default: {default_output})"
    )
    parser.add_argument(
        "--force", "-f", action="store_true",
        help="Overwrite output file if it already exists"
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    if not os.path.isfile(args.input):
        print(f"[ERROR] Input model not found: {args.input}")
        sys.exit(1)

    if os.path.isfile(args.output) and not args.force:
        print(f"[SKIP] Output already exists: {args.output}  (use --force to overwrite)")
        sys.exit(0)

    print(f"[INFO] Loading model: {args.input}")
    model = onnx.load(args.input)

    # Validate the model before conversion
    onnx.checker.check_model(model)
    print("[INFO] Model validation passed")

    # Report original size
    orig_size = os.path.getsize(args.input)
    print(f"[INFO] Original model size: {orig_size / 1024:.1f} KB")

    # Convert to FP16
    print("[INFO] Converting to FP16...")
    model_fp16 = float16.convert_float_to_float16(model)

    # Validate the converted model
    onnx.checker.check_model(model_fp16)
    print("[INFO] FP16 model validation passed")

    # Save
    os.makedirs(os.path.dirname(os.path.abspath(args.output)), exist_ok=True)
    onnx.save(model_fp16, args.output)

    new_size = os.path.getsize(args.output)
    print(f"[INFO] FP16 model saved: {args.output}")
    print(f"[INFO] FP16 model size: {new_size / 1024:.1f} KB  "
          f"({100.0 * new_size / orig_size:.0f}% of original)")


if __name__ == "__main__":
    main()
