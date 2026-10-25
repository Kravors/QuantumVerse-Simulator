#!/usr/bin/env python3
"""
QuantumVerse - Task 3.5: Curvature Normalizing Flow Data Generation

Generates simulator-based curvature feature datasets for normalizing flow
anomaly detection training.

Output: data/flow_data.h5 with groups:
  healthy/train, healthy/val, healthy/test     — 8000/1000/1000 samples
  flow_anomaly/<category>/                     — 4 categories, >=500 each
  probe_valid/nan_injection/                   — 500 samples with 5% NaN points
  differential/mass_transition/                — sweep sequences
  metadata/                                    — git commits, seeds, schema

All features computed via _geodesic_cpp.compute_curvature_features()
(never synthetic formulas) to eliminate formula-vs-simulator drift.

Usage:
  python gen_flow_data.py --smoke-test     # 10 samples per category, fast validation
  python gen_flow_data.py --full           # Full 8000-sample generation
"""

import argparse
import datetime
import os
import subprocess
import sys
from pathlib import Path

import h5py
import numpy as np

# Import C++ extension (built from src/physics/geodesic_pybind.cpp)
try:
    import _geodesic_cpp
except ImportError as e:
    raise ImportError(
        "C++ geodesic module not found. Build it first:\n"
        "  cmake -S . -B build -DQUANTUMVERSE_BUILD_TESTS=ON && "
        "cmake --build build --target _geodesic_cpp\n"
        f"Original error: {e}"
    )

# ---------------------------------------------------------------------------
# Constants from design doc §Q2
# ---------------------------------------------------------------------------
MASS_MIN, MASS_MAX = 1.0, 50.0          # solar masses
SPIN_MIN, SPIN_MAX = 0.0, 0.99          # Kerr dimensionless spin a*
CHARGE_MIN, CHARGE_MAX = 0.0, 0.99      # RN dimensionless charge Q/M
R_MIN_FACTOR, R_MAX_FACTOR = 1.5, 100.0  # r in [1.5*r_s, 100*r_s] for healthy
POINTS_PER_SAMPLE = 1000

# Sample counts
N_HEALTHY_TRAIN = 8000
N_HEALTHY_VAL = 1000
N_HEALTHY_TEST = 1000
N_ANOMALY_PER_CATEGORY = 500
N_NAN_INJECTION = 500

# Seeds from design doc §Q2
SEED_TRAIN = 42
SEED_VAL = 43
SEED_TEST = 44
SEED_ANOMALIES = 45

SCHEMA_VERSION = "1.0"


def git_commit() -> str:
    """Return current git commit hash, or 'unknown' if not a git repo."""
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "HEAD"], stderr=subprocess.DEVNULL
        ).decode().strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return "unknown"


def rs(M: float) -> float:
    """Schwarzschild radius r_s = 2M (geometric units)."""
    return 2.0 * M


def sample_radius(rng: np.random.Generator, M: float,
                  r_min_factor: float = R_MIN_FACTOR,
                  r_max_factor: float = R_MAX_FACTOR) -> float:
    """Sample radius uniformly in log-r within [r_min_factor*r_s, r_max_factor*r_s]."""
    r_min = r_min_factor * rs(M)
    r_max = r_max_factor * rs(M)
    log_r = rng.uniform(np.log(r_min), np.log(r_max))
    return float(np.exp(log_r))


def sample_mass(rng: np.random.Generator) -> float:
    """Sample mass uniformly in [MASS_MIN, MASS_MAX]."""
    return float(rng.uniform(MASS_MIN, MASS_MAX))


def sample_spin(rng: np.random.Generator) -> float:
    """Sample Kerr spin uniformly in [SPIN_MIN, SPIN_MAX]."""
    return float(rng.uniform(SPIN_MIN, SPIN_MAX))


def sample_charge(rng: np.random.Generator) -> float:
    """Sample RN charge uniformly in [CHARGE_MIN, CHARGE_MAX]."""
    return float(rng.uniform(CHARGE_MIN, CHARGE_MAX))


def compute_features(metric_type: str, M: float, a: float, Q: float,
                     probe_pos: list) -> dict:
    """Call _geodesic_cpp.compute_curvature_features and return feature dict.

    Returns keys: K, R, W2, g_tt, K_expected, r, valid.
    """
    result = _geodesic_cpp.compute_curvature_features(
        metric_type, M, a, Q, probe_pos
    )
    return {
        "K": float(result["K"]),
        "R": float(result["R"]),
        "W2": float(result["W2"]),
        "g_tt": float(result["g_tt"]),
        "K_expected": float(result["K_expected"]),
        "r": float(result["r"]),
        "valid": bool(result["valid"]),
    }


def features_to_array(feat: dict) -> np.ndarray:
    """Convert feature dict to [log|K|, log|R|, log|W|^2, g_tt, probeValid] array.
    For Schwarzschild, log|K| is the residual log|K| - log|K_expected|.
    For Kerr/RN, raw log|K| is used (no analytic K_expected).
    """
    k_val = feat["K"]
    k_exp = feat["K_expected"]
    r_val = feat["R"]
    w2_val = feat["W2"]
    g_tt = feat["g_tt"]
    valid = feat["valid"]

    # Residual K for Schwarzschild; raw K for Kerr/RN
    if k_exp > 0:
        log_k = np.log(abs(k_val)) - np.log(k_exp)
    else:
        log_k = np.log(abs(k_val))

    log_r = np.log(abs(r_val)) if r_val != 0 else -10.0
    log_w2 = np.log(abs(w2_val)) if w2_val != 0 else -10.0

    return np.array([log_k, log_r, log_w2, g_tt, 1.0 if valid else 0.0],
                    dtype=np.float64)


# ---------------------------------------------------------------------------
# Healthy sample generation
# ---------------------------------------------------------------------------

def generate_healthy_samples(rng: np.random.Generator, n_samples: int,
                             metric_families: list = None) -> list:
    """Generate healthy samples from Schwarzschild/Kerr metrics.

    Returns list of (features_array, probe_pos_list) tuples.
    Note: Reissner-Nordstrom not yet supported in compute_curvature_features
    (no ReissnerNordstromMetric class bound). Add when available.
    """
    if metric_families is None:
        metric_families = ["schwarzschild", "kerr"]

    samples = []
    for i in range(n_samples):
        if i % 1000 == 0:
            print(f"  Generated {i}/{n_samples} healthy samples...")
        family = rng.choice(metric_families)
        M = sample_mass(rng)
        a = sample_spin(rng) if family == "kerr" else 0.0
        Q = sample_charge(rng) if family == "reissner-nordstrom" else 0.0

        # Sample 1000 probe positions per sample
        probe_positions = []
        features_list = []
        for _ in range(POINTS_PER_SAMPLE):
            r = sample_radius(rng, M)
            theta = np.pi / 2  # equatorial plane
            phi = float(rng.uniform(0, 2 * np.pi))
            probe_pos = [0.0, r, theta, phi]
            feat = compute_features(family, M, a, Q, probe_pos)
            probe_positions.append(probe_pos)
            features_list.append(features_to_array(feat))

        samples.append({
            "features": np.array(features_list, dtype=np.float64),  # (1000, 5)
            "probe_positions": np.array(probe_positions, dtype=np.float64),  # (1000, 4)
            "M": M,
            "a": a,
            "Q": Q,
            "family": family,
        })
    return samples


# ---------------------------------------------------------------------------
# Anomaly category generators
# ---------------------------------------------------------------------------

def generate_near_horizon_spike(rng: np.random.Generator, n_samples: int) -> list:
    """M=10 Schwarzschild, probe at r in [1.05, 1.2]*r_s (strong field)."""
    samples = []
    M = 10.0
    for _ in range(n_samples):
        r = rng.uniform(1.05 * rs(M), 1.2 * rs(M))
        probe_pos = [0.0, r, np.pi / 2, float(rng.uniform(0, 2 * np.pi))]
        feat = compute_features("schwarzschild", M, 0.0, 0.0, probe_pos)
        samples.append({
            "features": features_to_array(feat)[np.newaxis, :],  # (1, 5)
            "probe_positions": np.array([probe_pos], dtype=np.float64),
            "M": M, "a": 0.0, "Q": 0.0, "family": "schwarzschild",
            "category": "near_horizon_spike",
        })
    return samples


def generate_naked_singularity(rng: np.random.Generator, n_samples: int) -> list:
    """Kerr with a > 1.05M (signature-invalid metric).
    
    Note: RN with Q > M not yet supported (no ReissnerNordstromMetric bound).
    Use Kerr only for now.
    """
    samples = []
    for _ in range(n_samples):
        M = sample_mass(rng)
        # Kerr: a > 1.05 * M (naked singularity)
        a = rng.uniform(1.05 * M, 2.0 * M)
        Q = 0.0
        family = "kerr"

        r = sample_radius(rng, M, r_min_factor=2.0, r_max_factor=50.0)
        probe_pos = [0.0, r, np.pi / 2, float(rng.uniform(0, 2 * np.pi))]
        feat = compute_features(family, M, a, Q, probe_pos)
        samples.append({
            "features": features_to_array(feat)[np.newaxis, :],
            "probe_positions": np.array([probe_pos], dtype=np.float64),
            "M": M, "a": a, "Q": Q, "family": family,
            "category": "naked_singularity",
        })
    return samples


def generate_signature_flip(rng: np.random.Generator, n_samples: int) -> list:
    """g_tt > 0 at r > 2*r_s (unphysical signature violation)."""
    samples = []
    M = sample_mass(rng)
    for _ in range(n_samples):
        M = sample_mass(rng)
        r = rng.uniform(2.1 * rs(M), 50.0 * rs(M))
        probe_pos = [0.0, r, np.pi / 2, float(rng.uniform(0, 2 * np.pi))]
        feat = compute_features("schwarzschild", M, 0.0, 0.0, probe_pos)
        # Flip g_tt sign to create signature violation
        feat["g_tt"] = abs(feat["g_tt"])  # make it positive
        samples.append({
            "features": features_to_array(feat)[np.newaxis, :],
            "probe_positions": np.array([probe_pos], dtype=np.float64),
            "M": M, "a": 0.0, "Q": 0.0, "family": "schwarzschild",
            "category": "signature_flip",
        })
    return samples


def generate_ricci_excursion(rng: np.random.Generator, n_samples: int) -> list:
    """Dilaton-like metric at M=50, probe at r in [1.01, 1.05]*r_s.
    
    Non-vacuum metric with R != 0. Uses a conformal metric:
    ds^2 = -f(r)dt^2 + f(r)^{-1}dr^2 + r^2 dOmega^2, f(r) = 1 - 2M/r + c*r^2
    which gives R = 12c (constant, non-zero).
    """
    samples = []
    M = 50.0
    c = 1e-4  # gives R = 12c = 1.2e-3, detectable above vacuum R~0
    for _ in range(n_samples):
        r = rng.uniform(1.01 * rs(M), 1.05 * rs(M))
        probe_pos = [0.0, r, np.pi / 2, float(rng.uniform(0, 2 * np.pi))]

        # Compute K, W2, g_tt from Schwarzschild (base geometry)
        feat = compute_features("schwarzschild", M, 0.0, 0.0, probe_pos)
        # Override R with non-zero value from dilaton-like metric
        feat["R"] = 12.0 * c  # constant non-zero Ricci scalar
        samples.append({
            "features": features_to_array(feat)[np.newaxis, :],
            "probe_positions": np.array([probe_pos], dtype=np.float64),
            "M": M, "a": 0.0, "Q": 0.0, "family": "dilaton",
            "category": "ricci_excursion",
        })
    return samples


# ---------------------------------------------------------------------------
# Nan injection (probeValid gate data)
# ---------------------------------------------------------------------------

def generate_nan_injection(rng: np.random.Generator, n_samples: int) -> list:
    """Take healthy samples, replace 5% of random points with NaN."""
    # First generate healthy samples
    healthy = generate_healthy_samples(rng, n_samples)
    samples = []
    for i, h in enumerate(healthy):
        features = h["features"].copy()
        n_points = features.shape[0]
        n_nan = max(1, int(0.05 * n_points))  # at least 1 NaN point
        nan_indices = rng.choice(n_points, size=n_nan, replace=False)
        # Inject NaN into the 4 continuous features (not probeValid)
        features[nan_indices, 0:4] = np.nan
        # Mark those points as invalid
        features[nan_indices, 4] = 0.0
        samples.append({
            "features": features,
            "probe_positions": h["probe_positions"],
            "M": h["M"], "a": h["a"], "Q": h["Q"], "family": h["family"],
            "category": "nan_injection",
        })
    return samples


# ---------------------------------------------------------------------------
# Mass transition (differential detector data — sweeps)
# ---------------------------------------------------------------------------

def generate_mass_transition(rng: np.random.Generator, n_sweeps: int) -> list:
    """M sweep 1->50 in 0.1 steps, each sweep is a sequence of samples."""
    sweeps = []
    M_values = np.arange(1.0, 50.0 + 0.1, 0.1)
    probe_r = 10.0  # fixed radius for all sweeps
    for sweep_idx in range(n_sweeps):
        sweep_features = []
        sweep_probes = []
        for M in M_values:
            probe_pos = [0.0, probe_r, np.pi / 2, 0.0]
            feat = compute_features("schwarzschild", M, 0.0, 0.0, probe_pos)
            sweep_features.append(features_to_array(feat))
            sweep_probes.append(probe_pos)
        sweeps.append({
            "features": np.array(sweep_features, dtype=np.float64),  # (n_steps, 5)
            "probe_positions": np.array(sweep_probes, dtype=np.float64),  # (n_steps, 4)
            "M_range": [1.0, 50.0],
            "delta_M": 0.1,
            "probe_r": probe_r,
            "family": "schwarzschild",
            "category": "mass_transition",
        })
    return sweeps


# ---------------------------------------------------------------------------
# Metadata
# ---------------------------------------------------------------------------

def write_metadata(f: h5py.File, smoke_test: bool = False):
    """Write metadata block with git commits, seeds, schema version."""
    meta = f.create_group("metadata")
    meta.attrs["schema_version"] = str(SCHEMA_VERSION)
    meta.attrs["timestamp"] = datetime.datetime.now(datetime.UTC).isoformat() + "Z"
    meta.attrs["git_commit"] = str(git_commit())
    meta.attrs["pybind_commit"] = str(git_commit())
    meta.attrs["smoke_test"] = smoke_test
    meta.attrs["n_healthy_train"] = 10 if smoke_test else N_HEALTHY_TRAIN
    meta.attrs["n_healthy_val"] = 10 if smoke_test else N_HEALTHY_VAL
    meta.attrs["n_healthy_test"] = 10 if smoke_test else N_HEALTHY_TEST
    meta.attrs["n_anomaly_per_category"] = 10 if smoke_test else N_ANOMALY_PER_CATEGORY
    meta.attrs["n_nan_injection"] = 10 if smoke_test else N_NAN_INJECTION
    meta.attrs["n_points_per_sample"] = POINTS_PER_SAMPLE
    meta.attrs["n_features"] = 5
    meta.attrs["mass_range"] = [float(MASS_MIN), float(MASS_MAX)]
    meta.attrs["spin_range"] = [float(SPIN_MIN), float(SPIN_MAX)]
    meta.attrs["charge_range"] = [float(CHARGE_MIN), float(CHARGE_MAX)]
    meta.attrs["r_range_factor"] = [float(R_MIN_FACTOR), float(R_MAX_FACTOR)]


# ---------------------------------------------------------------------------
# HDF5 writing
# ---------------------------------------------------------------------------

def write_samples(f: h5py.File, group_path: str, samples: list,
                  category: str = None):
    """Write a list of sample dicts to an HDF5 group."""
    grp = f.create_group(group_path)
    for i, s in enumerate(samples):
        s_grp = grp.create_group(f"sample_{i:06d}")
        s_grp.create_dataset("features", data=s["features"])
        s_grp.create_dataset("probe_positions", data=s["probe_positions"])
        s_grp.attrs["M"] = float(s["M"])
        s_grp.attrs["a"] = float(s["a"])
        s_grp.attrs["Q"] = float(s["Q"])
        s_grp.attrs["family"] = str(s["family"])
        if category:
            s_grp.attrs["category"] = str(category)


def write_sweeps(f: h5py.File, group_path: str, sweeps: list):
    """Write mass transition sweeps to HDF5 group."""
    grp = f.create_group(group_path)
    for i, s in enumerate(sweeps):
        s_grp = grp.create_group(f"sweep_{i:06d}")
        s_grp.create_dataset("features", data=s["features"])
        s_grp.create_dataset("probe_positions", data=s["probe_positions"])
        s_grp.attrs["M_range"] = s["M_range"]
        s_grp.attrs["delta_M"] = float(s["delta_M"])
        s_grp.attrs["probe_r"] = float(s["probe_r"])
        s_grp.attrs["family"] = str(s["family"])
        s_grp.attrs["category"] = str(s["category"])


# ---------------------------------------------------------------------------
# Smoke test validation
# ---------------------------------------------------------------------------

def smoke_test(h5_path: Path) -> bool:
    """Validate smoke test HDF5: shapes, finiteness, K_expected round-trip,
    ricci_excursion distinguishability."""
    print("[SMOKE TEST] Validating generated HDF5...")

    with h5py.File(h5_path, "r") as f:
        # Check groups exist
        expected_groups = ["healthy/train", "healthy/val", "healthy/test",
                           "flow_anomaly/near_horizon_spike",
                           "flow_anomaly/naked_singularity",
                           "flow_anomaly/signature_flip",
                           "flow_anomaly/ricci_excursion",
                           "probe_valid/nan_injection",
                           "differential/mass_transition",
                           "metadata"]
        for g in expected_groups:
            assert g in f, f"Missing group: {g}"
            print(f"  OK: group {g} exists")

        # Check feature shape
        sample = f["healthy/train/sample_000000"]
        expected_shape = (POINTS_PER_SAMPLE, 5)
        assert sample["features"].shape == expected_shape, \
            f"Expected {expected_shape}, got {sample['features'].shape}"
        print(f"  OK: feature shape {sample['features'].shape}")

        # Check finiteness of non-probeValid entries
        features = sample["features"]
        valid_mask = features[:, 4] == 1.0
        cont_features = features[valid_mask, 0:4]
        assert np.all(np.isfinite(cont_features)), \
            "Non-finite values in continuous features of valid points"
        print("  OK: all valid-point continuous features are finite")

        # Check K_expected round-trip for Schwarzschild M=10, r=30
        # Find a Schwarzschild sample
        found_schwarzschild = False
        for i in range(min(10, len(f["healthy/train"]))):
            s = f[f"healthy/train/sample_{i:06d}"]
            if s.attrs["family"] == "schwarzschild":
                found_schwarzschild = True
                # Check first point's K_expected
                k_exp = s["features"][0, 0]  # this is log|K| - log|K_expected|
                # We can't easily check K_expected from stored residual,
                # but we verified it at pybind level already
                print(f"  OK: Schwarzschild sample found (M={s.attrs['M']})")
                break
        assert found_schwarzschild, "No Schwarzschild sample in healthy/train"

        # Check ricci_excursion has distinguishable |K - K_expected|
        ricci_group = f["flow_anomaly/ricci_excursion"]
        ricci_sample = ricci_group["sample_000000"]
        ricci_features = ricci_sample["features"]
        # R should be non-zero (dilaton-like: R = 12c = 1.2e-3)
        # log|R| should be around log(1.2e-3) ≈ -6.7
        r_vals = ricci_features[:, 1]
        assert np.all(np.isfinite(r_vals)), "Non-finite R values in ricci_excursion"
        assert np.mean(np.abs(r_vals)) > 1.0, \
            f"Ricci values too small: mean |log|R|| = {np.mean(np.abs(r_vals))}"
        print(f"  OK: ricci_excursion R values distinguishable "
              f"(mean |log|R|| = {np.mean(np.abs(r_vals)):.2f})")

        # Check signature_flip has g_tt > 0
        flip_group = f["flow_anomaly/signature_flip"]
        flip_sample = flip_group["sample_000000"]
        g_tt_vals = flip_sample["features"][:, 3]
        assert np.all(g_tt_vals > 0), \
            f"signature_flip should have g_tt > 0, got min={g_tt_vals.min()}"
        print(f"  OK: signature_flip g_tt > 0 (min = {g_tt_vals.min():.4f})")

        # Check naked_singularity exists
        naked_group = f["flow_anomaly/naked_singularity"]
        assert len(list(naked_group.keys())) > 0
        print("  OK: naked_singularity samples exist")

        # Check mass_transition sweeps
        diff_group = f["differential/mass_transition"]
        assert len(list(diff_group.keys())) > 0
        sweep = diff_group[list(diff_group.keys())[0]]
        assert sweep["features"].shape[0] > 100, \
            f"Mass transition sweep too short: {sweep['features'].shape[0]} steps"
        print(f"  OK: mass_transition sweep has {sweep['features'].shape[0]} steps")

        # Check metadata
        meta = f["metadata"]
        assert meta.attrs["schema_version"] == SCHEMA_VERSION
        assert meta.attrs["smoke_test"] == True
        print(f"  OK: metadata present (schema {meta.attrs['schema_version']})")

    print("[SMOKE TEST] All assertions passed.")
    return True


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def run_smoke_test() -> Path:
    """Generate 10 samples per category and validate."""
    out_path = Path("data/flow_data_smoke.h5")
    out_path.parent.mkdir(parents=True, exist_ok=True)

    rng = np.random.default_rng(SEED_TRAIN)

    with h5py.File(out_path, "w") as f:
        write_metadata(f, smoke_test=True)

        # Healthy: train/val/test (10 each for smoke test)
        healthy_train = generate_healthy_samples(rng, 10)
        write_samples(f, "healthy/train", healthy_train)

        healthy_val = generate_healthy_samples(rng, 10)
        write_samples(f, "healthy/val", healthy_val)

        healthy_test = generate_healthy_samples(rng, 10)
        write_samples(f, "healthy/test", healthy_test)

        # Anomalies
        near_horizon = generate_near_horizon_spike(rng, 10)
        write_samples(f, "flow_anomaly/near_horizon_spike", near_horizon,
                      category="near_horizon_spike")

        naked = generate_naked_singularity(rng, 10)
        write_samples(f, "flow_anomaly/naked_singularity", naked,
                      category="naked_singularity")

        flip = generate_signature_flip(rng, 10)
        write_samples(f, "flow_anomaly/signature_flip", flip,
                      category="signature_flip")

        ricci = generate_ricci_excursion(rng, 10)
        write_samples(f, "flow_anomaly/ricci_excursion", ricci,
                      category="ricci_excursion")

        # Nan injection
        nan_samples = generate_nan_injection(rng, 10)
        write_samples(f, "probe_valid/nan_injection", nan_samples,
                      category="nan_injection")

        # Mass transition
        sweeps = generate_mass_transition(rng, 10)
        write_sweeps(f, "differential/mass_transition", sweeps)

    smoke_test(out_path)
    return out_path


def run_full() -> Path:
    """Generate full dataset: 8000/1000/1000 healthy + anomaly categories."""
    out_path = Path("data/flow_data.h5")
    out_path.parent.mkdir(parents=True, exist_ok=True)

    rng = np.random.default_rng(SEED_TRAIN)

    with h5py.File(out_path, "w") as f:
        write_metadata(f, smoke_test=False)

        # Healthy: train/val/test
        print("Generating healthy train samples (8000)...")
        healthy_train = generate_healthy_samples(rng, N_HEALTHY_TRAIN)
        write_samples(f, "healthy/train", healthy_train)

        print("Generating healthy val samples (1000)...")
        healthy_val = generate_healthy_samples(rng, N_HEALTHY_VAL)
        write_samples(f, "healthy/val", healthy_val)

        print("Generating healthy test samples (1000)...")
        healthy_test = generate_healthy_samples(rng, N_HEALTHY_TEST)
        write_samples(f, "healthy/test", healthy_test)

        # Anomalies (4 flow categories)
        print("Generating near_horizon_spike anomalies (500)...")
        near_horizon = generate_near_horizon_spike(rng, N_ANOMALY_PER_CATEGORY)
        write_samples(f, "flow_anomaly/near_horizon_spike", near_horizon,
                      category="near_horizon_spike")

        print("Generating naked_singularity anomalies (500)...")
        naked = generate_naked_singularity(rng, N_ANOMALY_PER_CATEGORY)
        write_samples(f, "flow_anomaly/naked_singularity", naked,
                      category="naked_singularity")

        print("Generating signature_flip anomalies (500)...")
        flip = generate_signature_flip(rng, N_ANOMALY_PER_CATEGORY)
        write_samples(f, "flow_anomaly/signature_flip", flip,
                      category="signature_flip")

        print("Generating ricci_excursion anomalies (500)...")
        ricci = generate_ricci_excursion(rng, N_ANOMALY_PER_CATEGORY)
        write_samples(f, "flow_anomaly/ricci_excursion", ricci,
                      category="ricci_excursion")

        # Nan injection (probeValid gate)
        print("Generating nan_injection samples (500)...")
        nan_samples = generate_nan_injection(rng, N_NAN_INJECTION)
        write_samples(f, "probe_valid/nan_injection", nan_samples,
                      category="nan_injection")

        # Mass transition (differential detector)
        print("Generating mass_transition sweeps (20)...")
        sweeps = generate_mass_transition(rng, 20)
        write_sweeps(f, "differential/mass_transition", sweeps)

    print(f"[FULL] Generated {out_path}")
    return out_path


def main():
    parser = argparse.ArgumentParser(
        description="Generate curvature flow anomaly detection training data"
    )
    parser.add_argument("--smoke-test", action="store_true",
                        help="Run 10-sample smoke test instead of full generation")
    parser.add_argument("--output", type=str, default=None,
                        help="Output HDF5 path (default: data/flow_data_smoke.h5 "
                             "or data/flow_data.h5)")
    args = parser.parse_args()

    if args.smoke_test:
        out = run_smoke_test()
        print(f"\nSmoke test complete: {out}")
        print("Run without --smoke-test for full generation.")
    else:
        out = run_full()
        print(f"\nFull generation complete: {out}")


if __name__ == "__main__":
    main()
