#!/usr/bin/env python3
"""
QuantumVerse - Task 2.1: Geodesic Neural ODE Surrogate
Fast Data Generation using C++ Backend via pybind11

This module provides a fast C++ implementation of Schwarzschild geodesic integration
using pybind11, replacing the slow pure Python RK4 implementation.

Author: dhiaeddine0223
Date: 2026-04-28
"""

import numpy as np
import h5py
import json
from typing import List, Tuple, Dict
from pathlib import Path
import argparse

# Import C++ extension (built from src/physics/geodesic_pybind.cpp)
try:
    import _geodesic_cpp
except ImportError as e:
    raise ImportError(
        "C++ geodesic module not found. Build it first:\n"
        "  cd f:/syyyy && cmake -S . -B build && cmake --build build --target geodesic_pybind\n"
        f"Original error: {e}"
    )

def sample_initial_conditions(rng: np.random.Generator, M: float) -> Tuple[np.ndarray, np.ndarray, float]:
    """
    Sample random initial event, velocity, and proper time for Schwarzschild geodesic.
    Returns: (initial_event [4], initial_velocity [4], tau_max)
    """
    # Sample radius (log-uniform for better coverage: 6M to 100M)
    r_min = 6.0 * M
    r_max = 100.0 * M
    log_r = rng.uniform(np.log(r_min), np.log(r_max))
    r = np.exp(log_r)
    
    # Equatorial orbit (θ = π/2)
    theta = np.pi / 2
    phi = rng.uniform(0, 2 * np.pi)
    
    # Initial event: t=0 (arbitrary origin)
    initial_event = np.array([0.0, r, theta, phi])
    
    # Compute initial velocity for a particle dropped from rest at radius r
    # In Schwarzschild, "at rest" means dr/dτ = 0 initially
    # From normalization: g_tt (u^t)^2 + g_rr (u^r)^2 = -1
    # With u^r = 0: (u^t)^2 = -1/g_tt = 1/(1 - 2M/r)
    rs = 2.0 * M
    u_t_squared = 1.0 / (1 - rs / r)
    ut = np.sqrt(u_t_squared)
    ur = 0.0
    uth = 0.0
    uph = 0.0
    
    initial_velocity = np.array([ut, ur, uth, uph])
    
    # Sample proper time integration duration
    tau = rng.uniform(0.1, 10.0)
    
    return initial_event, initial_velocity, tau

def generate_schwarzschild_sample(rng: np.random.Generator, M: float) -> Dict:
    """Generate a single geodesic sample for Schwarzschild metric using C++ integrator."""
    initial_event, initial_velocity, tau = sample_initial_conditions(rng, M)
    
    # Integrate using fast C++ backend
    # _geodesic_cpp.integrate_schwarzschild returns final state as numpy array
    final_state = _geodesic_cpp.integrate_schwarzschild(
        initial_event, initial_velocity, M, tau, num_steps=1000
    )
    final_event = final_state[0:4]  # Extract position (first 4 components)
    
    return {
        'initial_event': initial_event,
        'initial_velocity': initial_velocity,
        'metric_params': np.array([M]),
        'tau': tau,
        'final_event': final_event,
        'M': M
    }

def generate_dataset(output_path: Path, num_samples: int, metric_types: List[str]):
    """Generate full dataset and save to HDF5 using C++ integrator."""
    rng = np.random.default_rng(seed=42)  # reproducible
    
    # For now, only Schwarzschild
    samples = []
    M_range = (5.0, 50.0)  # solar masses (geometric units)
    
    print(f"Generating {num_samples} geodesic samples using C++ backend...")
    for i in range(num_samples):
        M = rng.uniform(M_range[0], M_range[1])
        sample = generate_schwarzschild_sample(rng, M)
        samples.append(sample)
        
        if (i+1) % 10000 == 0:
            print(f"  Generated {i+1}/{num_samples}")
    
    # Save to HDF5
    print(f"Saving dataset to {output_path}...")
    with h5py.File(output_path, 'w') as f:
        # Store as datasets
        f.create_dataset('initial_event', data=np.array([s['initial_event'] for s in samples]))
        f.create_dataset('initial_velocity', data=np.array([s['initial_velocity'] for s in samples]))
        f.create_dataset('metric_params', data=np.array([s['metric_params'] for s in samples]))
        f.create_dataset('tau', data=np.array([s['tau'] for s in samples]))
        f.create_dataset('final_event', data=np.array([s['final_event'] for s in samples]))
        
        # Store metadata
        meta = {
            'num_samples': num_samples,
            'metric_types': metric_types,
            'M_range': M_range,
            'tau_range': [0.1, 10.0],
            'r_range': [6.0, 100.0],
            'generator': 'python/datagen.py (C++ backend)',
            'integrator': 'RK4 (1000 steps, C++ implementation)'
        }
        f.attrs['metadata'] = json.dumps(meta)
    
    print(f"Dataset saved: {output_path} ({output_path.stat().st_size / 1e6:.1f} MB)")
    return str(output_path)

def main():
    parser = argparse.ArgumentParser(description="Generate geodesic dataset for neural ODE training (C++ backend)")
    parser.add_argument('--output', type=str, required=True, help='Output HDF5 file path')
    parser.add_argument('--samples', type=int, default=1000000, help='Number of geodesics to generate')
    parser.add_argument('--metrics', nargs='+', default=['Schwarzschild'], help='Metric types to include')
    args = parser.parse_args()
    
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    generate_dataset(output_path, args.samples, args.metrics)
    print("Dataset generation complete.")

if __name__ == '__main__':
    main()