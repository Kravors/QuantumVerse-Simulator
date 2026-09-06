#!/usr/bin/env python3
"""
generate_synthetic_training_data.py
====================================
Generate synthetic training data for the QuantumVerse anomaly detector.

Creates normal samples around typical solar system telemetry and anomalous
samples with injected distortions. Outputs JSON compatible with the C++
TrainingDataCollector format.
"""

import json
import random
import sys
import argparse
from pathlib import Path

random.seed(42)


def make_normal_sample(t: float) -> dict:
    return {
        "timestamp": t,
        "scenarioName": "synthetic_normal",
        "isAnomalous": False,
        "labelSource": "synthetic",
        "description": "normal telemetry",
        "features": [
            1.0e30 + random.uniform(-1e29, 1e29),   # totalKineticEnergy
           -2.0e30 + random.uniform(-1e29, 1e29),   # totalPotentialEnergy
           -1.0e30 + random.uniform(-1e29, 1e29),   # totalEnergy
            1.0e40 + random.uniform(-1e39, 1e39),   # angularMomentumZ
            29780.0 + random.uniform(-500, 500),     # earthOrbitalSpeed
            1.496e11 + random.uniform(-1e9, 1e9),    # earthDistance
            1.0e-21 + random.uniform(-1e-22, 1e-22), # gravitationalWaveStrain
            0.5 + random.uniform(-0.05, 0.05),       # gravitationalWaveFreq
            6.67430e-11,                               # G
            299792458.0,                               # c
            1.989e30 + random.uniform(-1e28, 1e28),   # blackHoleMass
            2.0 + random.uniform(-0.5, 0.5),          # bodyCount (float cast)
            30000.0 + random.uniform(-1000, 1000),    # meanBodySpeed
            40000.0 + random.uniform(-1000, 1000),    # maxBodySpeed
            20000.0 + random.uniform(-1000, 1000),    # minBodySpeed
            5000.0 + random.uniform(-500, 500),       # stdBodySpeed
            0.1 + random.uniform(-0.05, 0.05),        # meanInstrumentConfidence
            0.3 + random.uniform(-0.1, 0.1),          # maxInstrumentConfidence
            0.0,                                       # anomalyCount
            11.0 + random.uniform(-2, 2),             # instrumentCount
        ],
    }


def make_anomalous_sample(t: float) -> dict:
    s = make_normal_sample(t)
    s["isAnomalous"] = True
    s["labelSource"] = "synthetic"
    s["description"] = "injected anomaly"
    # Distort features strongly
    s["features"][0] *= 10.0   # KE
    s["features"][1] *= 5.0    # PE
    s["features"][2] *= 8.0    # Total E
    s["features"][6] *= 100.0  # GW strain
    s["features"][7] *= 10.0   # GW freq
    s["features"][12] *= 5.0   # mean speed
    s["features"][13] *= 5.0   # max speed
    s["features"][16] = 0.9    # high confidence
    s["features"][18] = 3.0     # anomaly count
    return s


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", default="data/ml/synthetic_training_data.json")
    parser.add_argument("--normal", type=int, default=1000)
    parser.add_argument("--anomalous", type=int, default=200)
    args = parser.parse_args()

    samples = []
    t = 0.0
    for _ in range(args.normal):
        samples.append(make_normal_sample(t))
        t += 1.0
    for _ in range(args.anomalous):
        samples.append(make_anomalous_sample(t))
        t += 1.0

    random.shuffle(samples)

    out = {
        "config": {"sampleInterval": 1.0, "maxSamples": len(samples)},
        "totalSamples": len(samples),
        "samples": samples,
    }

    path = Path(args.output)
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w") as f:
        json.dump(out, f, indent=2)

    print(f"Wrote {len(samples)} samples to {path}")


if __name__ == "__main__":
    main()
