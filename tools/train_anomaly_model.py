#!/usr/bin/env python3
"""
train_anomaly_model.py
======================
Train a lightweight autoencoder on QuantumVerse feature vectors and export
the model to JSON for the C++ AnomalyDetector runtime.

Architecture (must match C++ AnomalyDetector):
  Encoder: 20 -> 10 (ReLU) -> 2 (linear)
  Decoder:  2 -> 10 (ReLU) -> 20 (linear)

Usage:
  python tools/train_anomaly_model.py data/ml/training_data.json
  python tools/train_anomaly_model.py data/ml/training_data.json --output data/ml/anomaly_model.json
"""

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from scipy.stats import percentileofscore

try:
    import torch
    import torch.nn as nn
    import torch.optim as optim
    from torch.utils.data import DataLoader, TensorDataset
except ImportError:
    print("ERROR: PyTorch is required. Install with: pip install torch numpy scipy")
    sys.exit(1)


# ---------------------------------------------------------------------------
# Model definition (mirrors C++ AnomalyDetector architecture)
# ---------------------------------------------------------------------------

class Autoencoder(nn.Module):
    def __init__(self, feature_dim: int = 20):
        super().__init__()
        self.encoder = nn.Sequential(
            nn.Linear(feature_dim, 10),
            nn.ReLU(),
            nn.Linear(10, 2),
        )
        self.decoder = nn.Sequential(
            nn.Linear(2, 10),
            nn.ReLU(),
            nn.Linear(10, feature_dim),
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        z = self.encoder(x)
        return self.decoder(z)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def load_training_data(path: str):
    with open(path, "r") as f:
        data = json.load(f)

    samples = data.get("samples", [])
    if not samples:
        raise ValueError("No samples found in training data")

    features = []
    labels = []
    for s in samples:
        f = s.get("features", [])
        if len(f) != 20:
            raise ValueError(f"Expected 20 features, got {len(f)} in sample at t={s.get('timestamp')}")
        features.append(f)
        labels.append(1 if s.get("isAnomalous", False) else 0)

    return np.array(features, dtype=np.float64), np.array(labels, dtype=np.int64)


def compute_normalization(features: np.ndarray):
    mean = features.mean(axis=0).tolist()
    std = features.std(axis=0).tolist()
    std = [s if s > 1e-12 else 1.0 for s in std]
    return mean, std


def apply_normalization(features: np.ndarray, mean, std):
    mean = np.array(mean, dtype=np.float64)
    std = np.array(std, dtype=np.float64)
    return (features - mean) / std


def train_test_split_numpy(features, labels, test_size=0.2, random_state=42, stratify=True):
    rng = np.random.RandomState(random_state)
    n = len(features)
    indices = np.arange(n)
    if stratify:
        train_idx = []
        test_idx = []
        classes = np.unique(labels)
        for c in classes:
            cls_idx = indices[labels == c]
            rng.shuffle(cls_idx)
            split = int(len(cls_idx) * (1 - test_size))
            train_idx.append(cls_idx[:split])
            test_idx.append(cls_idx[split:])
        train_idx = np.concatenate(train_idx)
        test_idx = np.concatenate(test_idx)
    else:
        rng.shuffle(indices)
        split = int(n * (1 - test_size))
        train_idx = indices[:split]
        test_idx = indices[split:]
    return features[train_idx], features[test_idx], labels[train_idx], labels[test_idx]


def compute_metrics(labels, preds, scores):
    tp = int(np.sum((labels == 1) & (preds == 1)))
    fp = int(np.sum((labels == 0) & (preds == 1)))
    fn = int(np.sum((labels == 1) & (preds == 0)))
    tn = int(np.sum((labels == 0) & (preds == 0)))

    acc = (tp + tn) / max(tp + tn + fp + fn, 1)
    prec = tp / max(tp + fp, 1)
    rec = tp / max(tp + fn, 1)
    f1 = 2 * prec * rec / max(prec + rec, 1e-12)

    try:
        order = np.argsort(scores)
        labels_sorted = labels[order]
        n_pos = np.sum(labels == 1)
        n_neg = len(labels) - n_pos
        if n_pos == 0 or n_neg == 0:
            auc = 0.0
        else:
            ranks = np.arange(1, len(scores) + 1)[order]
            rank_sum_pos = np.sum(ranks[labels_sorted == 1])
            auc = (rank_sum_pos - n_pos * (n_pos + 1) / 2) / (n_pos * n_neg)
    except Exception:
        auc = 0.0

    return {
        "accuracy": float(acc),
        "precision": float(prec),
        "recall": float(rec),
        "f1": float(f1),
        "roc_auc": float(auc),
    }


def export_model(
    model: Autoencoder,
    feature_dim: int,
    threshold: float,
    mean,
    std,
    output_path: str,
):
    model.cpu()
    model.eval()

    def layer_to_dict(layer: nn.Linear) -> dict:
        w = layer.weight.detach().cpu().numpy()
        b = layer.bias.detach().cpu().numpy()
        # PyTorch stores weights as [out, in]; C++ expects weights[i][j] = W_ij
        # where i indexes output neuron and j indexes input neuron.
        # PyTorch layout already matches that convention.
        return {
            "in": int(w.shape[1]),
            "out": int(w.shape[0]),
            "weights": w.tolist(),
            "bias": b.tolist(),
        }

    encoder_layers = []
    for module in model.encoder:
        if isinstance(module, nn.Linear):
            encoder_layers.append(layer_to_dict(module))

    decoder_layers = []
    for module in model.decoder:
        if isinstance(module, nn.Linear):
            decoder_layers.append(layer_to_dict(module))

    export = {
        "feature_dim": feature_dim,
        "threshold": float(threshold),
        "mean": mean,
        "std": std,
        "encoder": encoder_layers,
        "decoder": decoder_layers,
    }

    out = Path(output_path)
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "w") as f:
        json.dump(export, f, indent=2)
    print(f"Model exported to {out}")


# ---------------------------------------------------------------------------
# Training loop
# ---------------------------------------------------------------------------

def train_autoencoder(
    x_train: np.ndarray,
    x_val: np.ndarray,
    epochs: int = 200,
    batch_size: int = 32,
    lr: float = 1e-3,
    patience: int = 20,
):
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model = Autoencoder(feature_dim=x_train.shape[1]).to(device)

    train_ds = TensorDataset(torch.tensor(x_train, dtype=torch.float32))
    val_ds = TensorDataset(torch.tensor(x_val, dtype=torch.float32))
    train_loader = DataLoader(train_ds, batch_size=batch_size, shuffle=True)
    val_loader = DataLoader(val_ds, batch_size=batch_size, shuffle=False)

    criterion = nn.MSELoss()
    optimizer = optim.Adam(model.parameters(), lr=lr)
    scheduler = optim.lr_scheduler.ReduceLROnPlateau(
        optimizer, mode="min", factor=0.5, patience=patience // 2
    )

    best_val_loss = float("inf")
    best_state = None
    wait = 0

    for epoch in range(epochs):
        model.train()
        for (batch,) in train_loader:
            batch = batch.to(device)
            optimizer.zero_grad()
            recon = model(batch)
            loss = criterion(recon, batch)
            loss.backward()
            optimizer.step()

        model.eval()
        val_losses = []
        with torch.no_grad():
            for (batch,) in val_loader:
                batch = batch.to(device)
                recon = model(batch)
                loss = criterion(recon, batch)
                val_losses.append(loss.item())

        val_loss = float(np.mean(val_losses))
        scheduler.step(val_loss)

        if epoch % 20 == 0 or epoch == epochs - 1:
            print(f"  epoch {epoch:03d}  val_loss={val_loss:.6f}")

        if val_loss < best_val_loss - 1e-8:
            best_val_loss = val_loss
            best_state = {k: v.cpu().clone() for k, v in model.state_dict().items()}
            wait = 0
        else:
            wait += 1
            if wait >= patience:
                print(f"  Early stopping at epoch {epoch}")
                break

    if best_state is not None:
        model.load_state_dict(best_state)
    return model


# ---------------------------------------------------------------------------
# Threshold selection
# ---------------------------------------------------------------------------

def select_threshold(model: Autoencoder, x_train: np.ndarray, percentile: float = 95.0):
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model.eval()
    with torch.no_grad():
        x = torch.tensor(x_train, dtype=torch.float32).to(device)
        recon = model(x).cpu().numpy()
    mse = ((x_train - recon) ** 2).mean(axis=1)
    threshold = float(np.percentile(mse, percentile))
    return threshold, mse


# ---------------------------------------------------------------------------
# Evaluation
# ---------------------------------------------------------------------------

def evaluate(model: Autoencoder, features: np.ndarray, labels: np.ndarray, threshold: float):
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model.eval()
    with torch.no_grad():
        x = torch.tensor(features, dtype=torch.float32).to(device)
        recon = model(x).cpu().numpy()
    mse = ((features - recon) ** 2).mean(axis=1)
    preds = (mse > threshold).astype(int)

    metrics = compute_metrics(labels, preds, mse)
    metrics["threshold"] = threshold
    metrics["mean_mse_normal"] = float(mse[labels == 0].mean()) if np.any(labels == 0) else 0.0
    metrics["mean_mse_anomalous"] = float(mse[labels == 1].mean()) if np.any(labels == 1) else 0.0
    return metrics


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="Train QuantumVerse anomaly autoencoder")
    parser.add_argument("input", help="Path to training_data.json from TrainingDataCollector")
    parser.add_argument("--output", default="data/ml/anomaly_model.json", help="Output model path")
    parser.add_argument("--epochs", type=int, default=200)
    parser.add_argument("--batch-size", type=int, default=32)
    parser.add_argument("--lr", type=float, default=1e-3)
    parser.add_argument("--threshold-percentile", type=float, default=95.0)
    parser.add_argument("--test-size", type=float, default=0.2)
    parser.add_argument("--random-state", type=int, default=42)
    args = parser.parse_args()

    print("Loading data...")
    features, labels = load_training_data(args.input)
    print(f"Loaded {len(features)} samples  (anomalous={labels.sum()}, normal={(labels==0).sum()})")

    mean, std = compute_normalization(features)
    features_norm = apply_normalization(features, mean, std)

    x_train, x_test, y_train, y_test = train_test_split_numpy(
        features_norm, labels, test_size=args.test_size, random_state=args.random_state
    )

    print("Training autoencoder...")
    model = train_autoencoder(
        x_train,
        x_test,
        epochs=args.epochs,
        batch_size=args.batch_size,
        lr=args.lr,
    )

    print("Selecting threshold...")
    threshold, train_mse = select_threshold(model, x_train, percentile=args.threshold_percentile)
    print(f"Threshold (P{args.threshold_percentile:.0f} of train MSE): {threshold:.6f}")

    print("Evaluating...")
    metrics = evaluate(model, features_norm, labels, threshold)
    print(f"  Accuracy : {metrics['accuracy']:.3f}")
    print(f"  Precision: {metrics['precision']:.3f}")
    print(f"  Recall   : {metrics['recall']:.3f}")
    print(f"  F1       : {metrics['f1']:.3f}")
    print(f"  ROC AUC  : {metrics['roc_auc']:.3f}")
    print(f"  Mean MSE normal      : {metrics['mean_mse_normal']:.6f}")
    print(f"  Mean MSE anomalous   : {metrics['mean_mse_anomalous']:.6f}")

    export_model(model, feature_dim=20, threshold=threshold, mean=mean, std=std, output_path=args.output)
    print("Done.")


if __name__ == "__main__":
    main()
