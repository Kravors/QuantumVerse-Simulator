#!/usr/bin/env python3
"""
Task 2.5: Curvature Dataset Generation for Normalizing Flow Anomaly Detection

Generates a dataset of curvature invariants from diverse GR spacetimes.
Each sample = one spacetime configuration (metric parameters) with feature vector
computed from curvature statistics at multiple sample points.

Spacetimes:
- Schwarzschild (M)
- Kerr (M, a)
- Reissner-Nordström (M, Q)
- Kerr-Newman (M, a, Q)
- Minkowski (vacuum)

Feature extraction:
- Sample N points in spacetime (r, theta, phi) outside horizon
- Compute curvature invariants at each point: R, K (Kretschmann), Weyl^2, Ricci^2
- Compute summary statistics: mean, std, min, max, percentiles, skew, kurtosis

Output: HDF5 file with features and labels
"""

import numpy as np
import h5py
import argparse
from tqdm import tqdm
import yaml
from pathlib import Path
from scipy import stats

# Physical constants (geometric units: G=c=1)
G = 1.0
C = 1.0

# ======================
# Curvature Invariant Computations
# ======================

def schwarzschild_curvature_invariants(M, r_vals):
    """
    Compute curvature invariants for Schwarzschild spacetime.
    
    Schwarzschild metric: ds² = -(1-2M/r)dt² + (1-2M/r)⁻¹dr² + r²dΩ²
    
    Curvature invariants:
    - Ricci scalar R = 0 (vacuum)
    - Kretschmann K = R_μνρσ R^μνρσ = 48 M² / r⁶
    - Weyl squared: C² = K (since Ricci=0 in vacuum, Weyl = Riemann)
    - Ricci squared: Ric2 = 0
    """
    rs = 2 * M  # Schwarzschild radius
    # Avoid r <= rs (inside horizon) - curvature singular at r=0
    valid = r_vals > rs * 1.01  # stay outside horizon with margin
    r = r_vals[valid]
    
    K = 48 * M**2 / r**6
    R = np.zeros_like(K)
    Weyl2 = K.copy()  # In vacuum, Weyl² = Kretschmann
    Ric2 = np.zeros_like(K)
    
    return R, K, Weyl2, Ric2, valid

def kerr_curvature_invariants(M, a, r_vals, theta_vals):
    """
    Compute curvature invariants for Kerr spacetime (rotating black hole).
    
    Parameters:
    - M: mass
    - a: angular momentum per unit mass (0 <= a <= M)
    - r, theta: Boyer-Lindquist coordinates
    
    Kretschmann for Kerr (exact formula):
    K = 48 M² (r² - a² cos²θ) (r⁴ - 14 a² r² cos²θ + a⁴ cos⁴θ) / (r² + a² cos²θ)⁶
    """
    rs = 2 * M
    # Ensure r outside horizon: r >= r_+ = M + sqrt(M² - a²)
    r_plus = M + np.sqrt(max(0, M**2 - a**2))
    valid = r_vals > r_plus * 1.01
    
    # For simplicity, sample at fixed theta values and average
    # More accurate: integrate over sphere or sample multiple theta
    R_list, K_list, W2_list, R2_list = [], [], [], []
    
    for r in r_vals[valid]:
        # Sample a few theta values
        thetas = np.linspace(0.1, np.pi-0.1, 5)
        for theta in thetas:
            a2 = a**2
            r2 = r**2
            cos2t = np.cos(theta)**2
            sin2t = np.sin(theta)**2
            
            sigma = r2 + a2 * cos2t
            delta = r2 - rs * r + a2
            
            # Kretschmann (Kerr) - simplified expression
            # Full expression is complex; using leading terms
            # For exact formula, see Chandrasekhar or MTW
            term1 = 48 * M**2 * (r2 - a2 * cos2t) * (r2**2 - 14 * a2 * r2 * cos2t + a2**2 * cos2t**2)
            denom = sigma**6
            K = term1 / denom
            
            # In vacuum: R=0, Ric2=0, Weyl2 ≈ K (but not exactly equal due to rotation)
            R = 0.0
            Ric2 = 0.0
            # Weyl² for Kerr is more complex; approximate as K for now
            Weyl2 = K
            
            R_list.append(R)
            K_list.append(K)
            W2_list.append(Weyl2)
            R2_list.append(Ric2)
    
    if len(R_list) == 0:
        return np.array([]), np.array([]), np.array([]), np.array([]), valid
    
    return (np.array(R_list), np.array(K_list), np.array(W2_list), 
            np.array(R2_list), valid)

def reissner_nordstrom_curvature_invariants(M, Q, r_vals):
    """
    Reissner-Nordström metric (charged black hole):
    ds² = -(1 - 2M/r + Q²/r²)dt² + (1 - 2M/r + Q²/r²)⁻¹ dr² + r²dΩ²
    
    Curvature invariants:
    - Ricci scalar R = 0 (electrovacuum)
    - Kretschmann K = 48 M²/r⁶ - 96 M Q²/r⁷ + 56 Q⁴/r⁸
    - Weyl² = K (since Ricci=0)
    - Ricci²: nonzero from electromagnetic field
    """
    # Avoid horizons: r >= r_+ = M + sqrt(M² - Q²)
    r_plus = M + np.sqrt(max(0, M**2 - Q**2))
    valid = r_vals > r_plus * 1.01
    r = r_vals[valid]
    
    K = 48 * M**2 / r**6 - 96 * M * Q**2 / r**7 + 56 * Q**4 / r**8
    R = np.zeros_like(K)
    Weyl2 = K.copy()
    # Ricci squared from electromagnetic field: Ric2 = 2 Q⁴ / r⁸ (in geometric units)
    Ric2 = 2 * Q**4 / r**8
    
    return R, K, Weyl2, Ric2, valid

def kerr_newman_curvature_invariants(M, a, Q, r_vals, theta_vals):
    """
    Kerr-Newman metric (rotating charged black hole).
    Most general stationary black hole solution.
    
    For now, use approximation: combine Kerr + RN effects.
    Full exact formula is extremely complex.
    """
    r_plus = M + np.sqrt(max(0, M**2 - a**2 - Q**2))
    valid = r_vals > r_plus * 1.01
    
    R_list, K_list, W2_list, R2_list = [], [], [], []
    
    for r in r_vals[valid]:
        thetas = np.linspace(0.1, np.pi-0.1, 5)
        for theta in thetas:
            # Simplified: add RN correction to Kerr
            # This is an approximation, not exact
            rs = 2 * M
            a2 = a**2
            r2 = r**2
            cos2t = np.cos(theta)**2
            sigma = r2 + a2 * cos2t
            delta = r2 - rs * r + a2 + Q**2
            
            # Kretschmann approximation
            term1 = 48 * M**2 * (r2 - a2 * cos2t) * (r2**2 - 14 * a2 * r2 * cos2t + a2**2 * cos2t**2)
            RN_correction = -96 * M * Q**2 / r**7 + 56 * Q**4 / r**8  # RN part
            K = term1 / sigma**6 + RN_correction
            
            R = 0.0  # Electrovacuum: R=0
            Ric2 = 2 * Q**4 / r**8  # EM contribution
            Weyl2 = K - Ric2  # Approximate decomposition: K = Weyl² + Ric2 (in 4D)
            
            R_list.append(R)
            K_list.append(K)
            W2_list.append(Weyl2)
            R2_list.append(Ric2)
    
    if len(R_list) == 0:
        return np.array([]), np.array([]), np.array([]), np.array([]), valid
    
    return (np.array(R_list), np.array(K_list), np.array(W2_list), 
            np.array(R2_list), valid)

def minkowski_curvature_invariants(r_vals):
    """Flat spacetime: all curvature invariants zero."""
    R = np.zeros_like(r_vals)
    K = np.zeros_like(r_vals)
    Weyl2 = np.zeros_like(r_vals)
    Ric2 = np.zeros_like(r_vals)
    valid = np.ones_like(r_vals, dtype=bool)
    return R, K, Weyl2, Ric2, valid

# ======================
# Feature Extraction
# ======================

def compute_statistics(values):
    """
    Compute summary statistics from array of curvature values.
    Returns feature vector of length 15. Handles edge cases (constant arrays, NaNs).
    """
    if len(values) == 0:
        return np.zeros(15)
    
    # Basic statistics
    mean = np.mean(values)
    std = np.std(values)
    min_val = np.min(values)
    max_val = np.max(values)
    
    # Percentiles
    p5, p25, p50, p75, p95 = np.percentile(values, [5, 25, 50, 75, 95])
    
    # Shape statistics - handle constant arrays (std=0) to avoid NaN
    if std < 1e-12 or np.allclose(values, values[0]):
        # All values identical (e.g., Minkowski K=0)
        skew = 0.0
        kurt = 0.0
    else:
        skew = stats.skew(values) if len(values) > 3 else 0.0
        kurt = stats.kurtosis(values) if len(values) > 3 else 0.0
        # Replace any NaN/Inf from edge cases
        if np.isnan(skew) or np.isinf(skew): skew = 0.0
        if np.isnan(kurt) or np.isinf(kurt): kurt = 0.0
    
    # Log-scale statistics (curvature spans many orders of magnitude)
    # Clip to avoid log(0) and overflow
    clipped = np.clip(values, 1e-30, 1e30)
    log_vals = np.log10(clipped)
    log_mean = np.mean(log_vals)
    log_std = np.std(log_vals)
    
    # Peakiness: max/mean, with epsilon to avoid division by zero
    peakiness = max_val / (abs(mean) + 1e-30)
    # RMS
    rms = np.sqrt(np.mean(values**2))
    
    # Combine
    features = np.array([
        mean, std, min_val, max_val,
        p5, p25, p50, p75, p95,
        skew, kurt,
        log_mean, log_std,
        peakiness,
        rms
    ])
    
    # Final safety: replace any remaining NaN/Inf with 0
    features = np.nan_to_num(features, nan=0.0, posinf=0.0, neginf=0.0)
    
    return features

# ======================
# Dataset Generation
# ======================

def generate_spacetime_samples(spacetime_type, n_samples_per_config, r_range=(5, 50), seed=42):
    """
    Generate curvature feature samples for one spacetime type.
    
    Returns:
    - features: (n_configs, feature_dim) array
    - labels: (n_configs,) integer labels
    - params: (n_configs, n_params) array of spacetime parameters
    """
    np.random.seed(seed)
    
    # Define parameter ranges for each spacetime
    if spacetime_type == 'schwarzschild':
        # M in [0.5, 5.0] (geometric units)
        M_vals = np.random.uniform(0.5, 5.0, n_samples_per_config)
        labels = np.zeros(n_samples_per_config, dtype=int)  # 0 = Schwarzschild
        params = M_vals.reshape(-1, 1)  # (n, 1): M
        
        features = []
        for M in tqdm(M_vals, desc=f"Schwarzschild (M∈[0.5,5.0])"):
            # Sample r values (outside horizon: r > 2M)
            r_min = max(5.0, 2*M * 1.1)
            r_max = 50.0
            r_vals = np.random.uniform(r_min, r_max, 100)
            
            R, K, Weyl2, Ric2, valid = schwarzschild_curvature_invariants(M, r_vals)
            if len(K) == 0:
                # Fallback: use zeros
                features.append(np.zeros(15))
                continue
            
            # Use Kretschmann as primary feature (most informative for vacuum)
            feat = compute_statistics(K)
            features.append(feat)
        
        features = np.stack(features)
        
    elif spacetime_type == 'kerr':
        # M in [1.0, 5.0], a/M in [0.0, 0.99] (avoid extremal for now)
        M_vals = np.random.uniform(1.0, 5.0, n_samples_per_config)
        a_vals = np.random.uniform(0.0, 0.99, n_samples_per_config) * M_vals  # a = spin * M
        labels = np.ones(n_samples_per_config, dtype=int)  # 1 = Kerr
        params = np.stack([M_vals, a_vals], axis=1)  # (n, 2): M, a
        
        features = []
        for M, a in tqdm(zip(M_vals, a_vals), desc=f"Kerr (M∈[1,5], a∈[0,0.99M])"):
            r_plus = M + np.sqrt(max(0, M**2 - a**2))
            r_min = r_plus * 1.1
            r_max = 50.0
            r_vals = np.random.uniform(r_min, r_max, 100)
            theta_vals = np.random.uniform(0.1, np.pi-0.1, 10)
            
            R, K, Weyl2, Ric2, valid = kerr_curvature_invariants(M, a, r_vals, theta_vals)
            if len(K) == 0:
                features.append(np.zeros(15))
                continue
            
            feat = compute_statistics(K)
            features.append(feat)
        
        features = np.stack(features)
        
    elif spacetime_type == 'reissner_nordstrom':
        # M in [1.0, 5.0], Q in [0.1, M] (Q <= M for no naked singularity)
        M_vals = np.random.uniform(1.0, 5.0, n_samples_per_config)
        Q_vals = np.random.uniform(0.1, 0.99, n_samples_per_config) * M_vals  # Q <= M
        labels = 2 * np.ones(n_samples_per_config, dtype=int)  # 2 = RN
        params = np.stack([M_vals, Q_vals], axis=1)  # (n, 2): M, Q
        
        features = []
        for M, Q in tqdm(zip(M_vals, Q_vals), desc=f"Reissner-Nordström (M∈[1,5], Q∈[0.1,M])"):
            r_plus = M + np.sqrt(max(0, M**2 - Q**2))
            r_min = r_plus * 1.1
            r_max = 50.0
            r_vals = np.random.uniform(r_min, r_max, 100)
            
            R, K, Weyl2, Ric2, valid = reissner_nordstrom_curvature_invariants(M, Q, r_vals)
            if len(K) == 0:
                features.append(np.zeros(15))
                continue
            
            feat = compute_statistics(K)
            features.append(feat)
        
        features = np.stack(features)
        
    elif spacetime_type == 'kerr_newman':
        # M in [1.0, 4.0], a/M in [0.0, 0.9], Q/M in [0.1, 0.9]
        # Constraint: a² + Q² <= M² (cosmic censorship)
        M_vals = np.random.uniform(1.0, 4.0, n_samples_per_config)
        a_vals = []
        Q_vals = []
        for M in M_vals:
            # Sample a, Q respecting a² + Q² <= M²
            max_a = M * 0.9
            max_Q = M * 0.9
            a = np.random.uniform(0, max_a)
            Q_max = np.sqrt(M**2 - a**2)
            Q = np.random.uniform(0.1, min(max_Q, max_Q))
            a_vals.append(a)
            Q_vals.append(Q)
        a_vals = np.array(a_vals)
        Q_vals = np.array(Q_vals)
        labels = 3 * np.ones(n_samples_per_config, dtype=int)  # 3 = KN
        params = np.stack([M_vals, a_vals, Q_vals], axis=1)  # (n, 3): M, a, Q
        
        features = []
        for M, a, Q in tqdm(zip(M_vals, a_vals, Q_vals), desc=f"Kerr-Newman (M∈[1,4], a,Q∈[0.1,0.9])"):
            r_plus = M + np.sqrt(max(0, M**2 - a**2 - Q**2))
            r_min = r_plus * 1.1
            r_max = 50.0
            r_vals = np.random.uniform(r_min, r_max, 100)
            theta_vals = np.random.uniform(0.1, np.pi-0.1, 5)
            
            R, K, Weyl2, Ric2, valid = kerr_newman_curvature_invariants(M, a, Q, r_vals, theta_vals)
            if len(K) == 0:
                features.append(np.zeros(15))
                continue
            
            feat = compute_statistics(K)
            features.append(feat)
        
        features = np.stack(features)
        
    elif spacetime_type == 'minkowski':
        # Flat spacetime: no parameters
        labels = 4 * np.ones(n_samples_per_config, dtype=int)
        params = np.zeros((n_samples_per_config, 1))  # dummy
        features = []
        for _ in tqdm(range(n_samples_per_config), desc="Minkowski"):
            # Curvature is zero everywhere
            K = np.zeros(100)
            feat = compute_statistics(K)
            features.append(feat)
        features = np.stack(features)
    
    else:
        raise ValueError(f"Unknown spacetime type: {spacetime_type}")
    
    return features, labels, params

def generate_full_dataset(n_per_type=2000, output_path='data/curvature_dataset.h5'):
    """
    Generate full dataset with all spacetime types.
    
    Dataset composition:
    - 40% Schwarzschild (normal)
    - 30% Kerr (normal, rotating)
    - 15% Reissner-Nordström (normal, charged)
    - 10% Kerr-Newman (normal, rotating+charged)
    - 5% Minkowski (trivial normal)
    - (Exotic spacetimes like Hayward, Bardeen will be used as ANOMALIES during testing)
    """
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    all_features = []
    all_labels = []
    all_params = []
    
    # Generate each type
    type_configs = {
        'schwarzschild': int(n_per_type * 0.40),
        'kerr': int(n_per_type * 0.30),
        'reissner_nordstrom': int(n_per_type * 0.15),
        'kerr_newman': int(n_per_type * 0.10),
        'minkowski': int(n_per_type * 0.05),
    }
    
    print("Generating curvature dataset...")
    print(f"Total samples: {sum(type_configs.values())}")
    
    for spacetime_type, n_samples in type_configs.items():
        print(f"\n--- Generating {spacetime_type} ({n_samples} samples) ---")
        features, labels, params = generate_spacetime_samples(
            spacetime_type, n_samples, seed=np.random.randint(0, 10000)
        )
        all_features.append(features)
        all_labels.append(labels)
        all_params.append(params)
    
    # Concatenate features and labels
    features = np.concatenate(all_features, axis=0)
    labels = np.concatenate(all_labels, axis=0)
    
    # Pad params to same dimension (max 3: M, a, Q)
    max_params = 3
    padded_params = []
    for p in all_params:
        if p.shape[1] < max_params:
            # Pad with zeros
            padding = np.zeros((p.shape[0], max_params - p.shape[1]))
            padded = np.concatenate([p, padding], axis=1)
            padded_params.append(padded)
        else:
            padded_params.append(p)
    params = np.concatenate(padded_params, axis=0)
    
    # Shuffle
    perm = np.random.permutation(len(features))
    features = features[perm]
    labels = labels[perm]
    params = params[perm]
    
    # Save to HDF5
    print(f"\nSaving dataset to {output_path}...")
    with h5py.File(output_path, 'w') as f:
        f.create_dataset('features', data=features, compression='gzip')
        f.create_dataset('labels', data=labels, compression='gzip')
        f.create_dataset('params', data=params, compression='gzip')
        
        # Store metadata
        f.attrs['n_samples'] = len(features)
        f.attrs['feature_dim'] = features.shape[1]
        f.attrs['param_dim'] = params.shape[1]
        f.attrs['spacetime_types'] = yaml.dump(list(type_configs.keys()))
        f.attrs['description'] = 'Curvature invariant statistics for normalizing flow anomaly detection'
        # Parameter mapping: [M, a, Q] (unused params = 0)
        f.attrs['param_names'] = yaml.dump(['mass', 'spin', 'charge'])
    
    print(f"Dataset saved: {features.shape[0]} samples, {features.shape[1]} features, {params.shape[1]} params")
    print("Label mapping:")
    label_names = ['schwarzschild', 'kerr', 'reissner_nordstrom', 'kerr_newman', 'minkowski']
    for i, name in enumerate(label_names):
        count = np.sum(labels == i)
        print(f"  {i}: {name} ({count} samples)")
    
    return output_path

# ======================
# Main
# ======================

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Generate curvature dataset for anomaly detection')
    parser.add_argument('--n-per-type', type=int, default=2000, help='Samples per spacetime type')
    parser.add_argument('--output', type=str, default='data/curvature_dataset.h5', help='Output HDF5 file')
    args = parser.parse_args()
    
    generate_full_dataset(n_per_type=args.n_per_type, output_path=args.output)
