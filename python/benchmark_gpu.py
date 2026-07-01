"""
GPU Acceleration Benchmark for GeodesicNeuralODE ONNX Inference

Benchmarks GPU (CUDA) vs CPU performance with batched inference.
Target: NVIDIA GTX 1650 4GB VRAM
Uses onnxruntime 1.23.x compatible API
"""

import numpy as np
import onnxruntime as ort
import time
import json
import sys

MODEL_PATH = "models/geodesic_ode/geodesic_ode.onnx"
NORM_PATH = "models/geodesic_ode/normalization.json"
BATCH_SIZES = [1, 32, 64, 128, 256, 512, 1024, 2048, 4096]
NUM_WARMUP = 10
NUM_RUNS = 50
TOTAL_SAMPLES = 100000


def print_sep(title):
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}")


def main():
    # Load normalization params
    with open(NORM_PATH) as f:
        norm = json.load(f)

    print_sep("GPU Acceleration Benchmark for Geodesic Neural ODE")
    print(f"Model: {MODEL_PATH}")
    print(f"Input dim: {norm['input_dim']}")
    print(f"ONNX Runtime version: {ort.__version__}")

    # Check available providers
    available = ort.get_available_providers()
    print(f"\nAvailable providers: {available}")
    print(f"Device: {ort.get_device()}")

    # Create CPU session
    cpu_opts = ort.SessionOptions()
    cpu_opts.intra_op_num_threads = 1
    cpu_opts.inter_op_num_threads = 1
    cpu_opts.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_ALL
    cpu_session = ort.InferenceSession(
        MODEL_PATH, sess_options=cpu_opts,
        providers=['CPUExecutionProvider'])
    print("[CPU] Session created (1 thread)")

    # Create GPU session - use provider_options parameter
    gpu_opts = ort.SessionOptions()
    gpu_opts.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_ALL
    gpu_session = ort.InferenceSession(
        MODEL_PATH, sess_options=gpu_opts,
        providers=['CUDAExecutionProvider', 'CPUExecutionProvider'],
        provider_options=[{'device_id': '0', 'gpu_mem_limit': '2147483648'}, {}])
    print("[GPU] Session created (CUDA EP)")

    input_name = cpu_session.get_inputs()[0].name
    output_name = cpu_session.get_outputs()[0].name
    print(f"Input:  {input_name} shape={cpu_session.get_inputs()[0].shape}")
    print(f"Output: {output_name} shape={cpu_session.get_outputs()[0].shape}")

    # Benchmark different batch sizes (GPU only, CPU would take forever)
    print_sep("GPU Batch Size Benchmark")
    print(f"{'Batch':>6} | {'Latency (ms)':>13} | {'Throughput':>15}")
    print("-" * 40)

    best_throughput = 0
    best_batch_size = 1

    for batch_size in BATCH_SIZES:
        try:
            dummy = np.random.randn(batch_size, 10).astype(np.float32)
            for _ in range(NUM_WARMUP):
                gpu_session.run([output_name], {input_name: dummy})
            start = time.perf_counter()
            for _ in range(NUM_RUNS):
                gpu_session.run([output_name], {input_name: dummy})
            elapsed = time.perf_counter() - start
            avg_ms = (elapsed / NUM_RUNS) * 1000
            throughput = (batch_size * NUM_RUNS) / elapsed
            print(f"{batch_size:>6} | {avg_ms:>13.3f} | {throughput:>15.0f} sps")
            if throughput > best_throughput:
                best_throughput = throughput
                best_batch_size = batch_size
        except Exception as e:
            print(f"{batch_size:>6} | {'OOM/FAIL':>13} | {str(e)[:30]:>15}")

    print(f"\nOptimal batch size: {best_batch_size} ({best_throughput:.0f} samples/sec)")

    # Single sample comparison
    print_sep("Single-Sample Performance")
    dummy = np.random.randn(1, 10).astype(np.float32)
    for _ in range(NUM_WARMUP):
        cpu_session.run([output_name], {input_name: dummy})
    cpu_start = time.perf_counter()
    for _ in range(NUM_RUNS):
        cpu_session.run([output_name], {input_name: dummy})
    cpu_elapsed = time.perf_counter() - cpu_start
    cpu_ms = (cpu_elapsed / NUM_RUNS) * 1000

    for _ in range(NUM_WARMUP):
        gpu_session.run([output_name], {input_name: dummy})
    gpu_start = time.perf_counter()
    for _ in range(NUM_RUNS):
        gpu_session.run([output_name], {input_name: dummy})
    gpu_elapsed = time.perf_counter() - gpu_start
    gpu_ms = (gpu_elapsed / NUM_RUNS) * 1000

    print(f"  CPU single: {cpu_ms:.3f} ms ({1000/cpu_ms:.0f} sps)")
    print(f"  GPU single: {gpu_ms:.3f} ms ({1000/gpu_ms:.0f} sps)")
    print(f"  Speedup: {cpu_ms/gpu_ms:.1f}x")

    # Full validation estimate
    print_sep(f"Full Validation ({TOTAL_SAMPLES:,} samples)")
    cpu_total = TOTAL_SAMPLES / (1000 / cpu_ms)
    gpu_total = TOTAL_SAMPLES / best_throughput
    print(f"  CPU estimate: {cpu_total:.1f}s ({cpu_total/60:.1f} min)")
    print(f"  GPU estimate: {gpu_total:.1f}s ({gpu_total/60:.1f} min)")
    print(f"  Speedup: {cpu_total/gpu_total:.0f}x")
    if gpu_total < 60:
        print(f"  Result: {'PASS' if gpu_total < 120 else 'CHECK'} - validation < {gpu_total:.0f}s")

    # Correctness check
    print_sep("Correctness Check")
    test_input = np.random.randn(1, 10).astype(np.float32)
    cpu_out = cpu_session.run([output_name], {input_name: test_input})
    gpu_out = gpu_session.run([output_name], {input_name: test_input})
    max_diff = np.max(np.abs(cpu_out[0] - gpu_out[0]))
    print(f"  CPU vs GPU max diff: {max_diff:.6e}")
    ok = max_diff < 1e-5
    print(f"  {'PASS' if ok else 'FAIL - Large discrepancy detected'}")

    time.sleep(0.5)
    return ok


if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)