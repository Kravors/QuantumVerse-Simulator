#!/usr/bin/env python3
"""
QuantumVerse - Task 2.X: LightGBM Discovery Pipeline
Training Module for Anomaly Detection and Classification

This script trains LightGBM models for:
1. Anomaly detection in curvature patterns
2. Classification of exotic spacetime signatures
3. Discovery instrument data analysis

Architecture:
  - Objective: binary (anomaly detection) / multiclass (classification)
  - Boosting: gbdt (default) or dart for better generalisation
  - n_estimators: 1000 with early stopping (50 rounds)
  - learning_rate: 0.01-0.1 (lower for larger datasets)
  - num_leaves: 31-255 (tuned with optuna or grid search)
  - min_data_in_leaf: 20-100 (regularisation)
  - feature_fraction / bagging_fraction: 0.8 (speed and robustness)
  - reg_alpha / reg_lambda: 0.1-1.0 (L1/L2 regularisation)

Author: Kilo Code (AI Assistant)
Date: 2026-06-25
"""

import lightgbm as lgb
import numpy as np
import pandas as pd
import json
import argparse
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report, roc_auc_score
import optuna
from optuna.integration import LightGBMPruningCallback


def create_dataset_from_features(
    features: np.ndarray,
    labels: np.ndarray,
    categorical_features: Optional[List[int]] = None
) -> lgb.Dataset:
    """Create LightGBM dataset with optional categorical features."""
    return lgb.Dataset(
        features,
        label=labels,
        categorical_feature=categorical_features
    )


def get_optimized_params(
    objective: str,
    n_features: int,
    n_classes: int = 2,
    trial: Optional[optuna.Trial] = None
) -> Dict:
    """
    Get optimized LightGBM parameters.
    
    Args:
        objective: 'binary', 'multiclass', or 'regression'
        n_features: Number of input features
        n_classes: Number of classes (for multiclass)
        trial: Optuna trial for hyperparameter tuning
    
    Returns:
        Dictionary of LightGBM parameters
    """
    if trial is not None:
        # Optuna suggested parameters
        params = {
            'objective': objective,
            'boosting_type': trial.suggest_categorical('boosting_type', ['gbdt', 'dart']),
            'num_leaves': trial.suggest_int('num_leaves', 31, min(255, n_features * 2)),
            'max_depth': trial.suggest_int('max_depth', 3, 12),
            'learning_rate': trial.suggest_loguniform('learning_rate', 0.01, 0.3),
            'min_data_in_leaf': trial.suggest_int('min_data_in_leaf', 20, 100),
            'feature_fraction': trial.suggest_uniform('feature_fraction', 0.6, 0.9),
            'bagging_fraction': trial.suggest_uniform('bagging_fraction', 0.6, 0.9),
            'bagging_freq': trial.suggest_int('bagging_freq', 1, 10),
            'reg_alpha': trial.suggest_loguniform('reg_alpha', 0.0, 1.0),
            'reg_lambda': trial.suggest_loguniform('reg_lambda', 0.0, 1.0),
            'min_gain_to_split': trial.suggest_uniform('min_gain_to_split', 0.0, 1.0),
            'verbose': -1
        }
    else:
        # Default production parameters
        params = {
            'objective': objective,
            'boosting_type': 'gbdt',
            'num_leaves': 127,
            'max_depth': -1,  # No limit
            'learning_rate': 0.05,
            'min_data_in_leaf': 50,
            'feature_fraction': 0.8,
            'bagging_fraction': 0.8,
            'bagging_freq': 5,
            'reg_alpha': 0.1,
            'reg_lambda': 0.1,
            'min_gain_to_split': 0.01,
            'verbose': -1
        }
    
    if objective == 'multiclass':
        params['num_class'] = n_classes
    
    return params


def train_anomaly_detector(
    X_train: np.ndarray,
    y_train: np.ndarray,
    X_val: np.ndarray,
    y_val: np.ndarray,
    output_dir: Path,
    params: Optional[Dict] = None,
    use_optuna: bool = False,
    n_trials: int = 50
) -> Tuple[lgb.Booster, Dict]:
    """
    Train LightGBM model for anomaly detection.
    
    Args:
        X_train, y_train: Training data
        X_val, y_val: Validation data
        output_dir: Directory to save model
        params: Optional pre-defined parameters
        use_optuna: Whether to use Optuna for hyperparameter tuning
        n_trials: Number of Optuna trials
    
    Returns:
        Trained model and metadata
    """
    # Create datasets
    train_data = create_dataset_from_features(X_train, y_train)
    val_data = create_dataset_from_features(X_val, y_val)
    
    if params is None:
        params = get_optimized_params('binary', X_train.shape[1])
    
    # Optuna objective
    def optuna_objective(trial):
        opt_params = get_optimized_params('binary', X_train.shape[1], trial=trial)
        gbm = lgb.train(
            opt_params,
            train_data,
            valid_sets=[val_data],
            callbacks=[LightGBMPruningCallback(trial, 'binary_error')]
        )
        return gbm.eval(val_data)[0][1]
    
    if use_optuna:
        study = optuna.create_study(direction='minimize')
        study.optimize(optuna_objective, n_trials=n_trials)
        params = get_optimized_params('binary', X_train.shape[1], trial=study.best_trial)
    
    # Train final model
    model = lgb.train(
        params,
        train_data,
        num_boost_round=1000,
        valid_sets=[val_data],
        callbacks=[
            lgb.log_evaluation(period=100),
            lgb.early_stopping(stopping_rounds=50, verbose=True)
        ]
    )
    
    # Save model
    model_path = output_dir / 'lightgbm_anomaly_detector.txt'
    model.save_model(str(model_path))
    
    # Metadata
    metadata = {
        'model_type': 'anomaly_detector',
        'objective': 'binary',
        'params': params,
        'n_features': X_train.shape[1],
        'n_train_samples': len(X_train),
        'n_val_samples': len(X_val),
        'best_iteration': model.num_trees(),
        'date_trained': pd.Timestamp.now().isoformat()
    }
    
    with open(output_dir / 'anomaly_detector_metadata.json', 'w') as f:
        json.dump(metadata, f, indent=2)
    
    return model, metadata


def train_classifier(
    X_train: np.ndarray,
    y_train: np.ndarray,
    X_val: np.ndarray,
    y_val: np.ndarray,
    output_dir: Path,
    n_classes: int = 5,
    params: Optional[Dict] = None
) -> Tuple[lgb.Booster, Dict]:
    """
    Train LightGBM model for spacetime classification.
    
    Args:
        X_train, y_train: Training data
        X_val, y_val: Validation data
        output_dir: Directory to save model
        n_classes: Number of classes
        params: Optional pre-defined parameters
    
    Returns:
        Trained model and metadata
    """
    train_data = create_dataset_from_features(X_train, y_train)
    val_data = create_dataset_from_features(X_val, y_val)
    
    if params is None:
        params = get_optimized_params('multiclass', X_train.shape[1], n_classes)
    
    model = lgb.train(
        params,
        train_data,
        num_boost_round=1000,
        valid_sets=[val_data],
        callbacks=[
            lgb.log_evaluation(period=100),
            lgb.early_stopping(stopping_rounds=50, verbose=True)
        ]
    )
    
    # Save model
    model_path = output_dir / 'lightgbm_classifier.txt'
    model.save_model(str(model_path))
    
    # Evaluate
    y_pred = np.argmax(model.predict(X_val), axis=1)
    print(classification_report(y_val, y_pred))
    
    # Metadata
    metadata = {
        'model_type': 'classifier',
        'objective': 'multiclass',
        'num_classes': n_classes,
        'params': params,
        'n_features': X_train.shape[1],
        'n_train_samples': len(X_train),
        'n_val_samples': len(X_val),
        'best_iteration': model.num_trees(),
        'date_trained': pd.Timestamp.now().isoformat()
    }
    
    with open(output_dir / 'classifier_metadata.json', 'w') as f:
        json.dump(metadata, f, indent=2)
    
    return model, metadata


def generate_synthetic_discovery_data(
    n_samples: int = 10000,
    n_features: int = 15
) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    """
    Generate synthetic data for discovery instruments.
    
    Features: 15-dimensional curvature statistics
    Labels: 0=normal, 1=anomaly (binary) or class 0-4 (multiclass)
    """
    np.random.seed(42)
    
    # Generate features (curvature statistics)
    X = np.random.randn(n_samples, n_features)
    
    # Generate labels with some pattern
    # Anomaly if Kretschmann scalar is high
    anomaly_score = X[:, 0] * 2 + X[:, 1] * 0.5 + np.random.randn(n_samples) * 0.5
    y_binary = (anomaly_score > 1.0).astype(int)
    y_multiclass = np.clip((anomaly_score + 2).astype(int), 0, 4)
    
    return X, y_binary, y_multiclass


def main():
    parser = argparse.ArgumentParser(description="Train LightGBM Discovery Models")
    parser.add_argument('--data', type=str, default=None, help='Path to CSV/HDF5 data file')
    parser.add_argument('--output-dir', type=str, default='./models/lightgbm',
                        help='Output directory for models')
    parser.add_argument('--model-type', type=str, choices=['anomaly', 'classifier', 'both'],
                        default='both', help='Type of model to train')
    parser.add_argument('--n-classes', type=int, default=5, help='Number of classes for classifier')
    parser.add_argument('--use-optuna', action='store_true', help='Use Optuna for hyperparameter tuning')
    parser.add_argument('--n-trials', type=int, default=50, help='Number of Optuna trials')
    parser.add_argument('--n-samples', type=int, default=10000, help='Number of synthetic samples')
    args = parser.parse_args()
    
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # Load or generate data
    if args.data:
        # Load from file
        df = pd.read_csv(args.data)
        X = df.drop('label', axis=1).values
        y = df['label'].values
        y_binary = (y > 0).astype(int)
        y_multiclass = y
    else:
        # Generate synthetic data
        print(f"Generating {args.n_samples} synthetic samples...")
        X, y_binary, y_multiclass = generate_synthetic_discovery_data(args.n_samples)
    
    # Split data
    X_train, X_val, y_train_bin, y_val_bin = train_test_split(
        X, y_binary, test_size=0.2, random_state=42
    )
    _, _, y_train_multi, y_val_multi = train_test_split(
        X, y_multiclass, test_size=0.2, random_state=42
    )
    
    print(f"Training samples: {len(X_train)}, Validation samples: {len(X_val)}")
    
    # Train models
    if args.model_type in ['anomaly', 'both']:
        print("\n=== Training Anomaly Detector ===")
        anomaly_model, anomaly_meta = train_anomaly_detector(
            X_train, y_train_bin, X_val, y_val_bin,
            output_dir, use_optuna=args.use_optuna, n_trials=args.n_trials
        )
        print(f"Anomaly model saved to {output_dir / 'lightgbm_anomaly_detector.txt'}")
    
    if args.model_type in ['classifier', 'both']:
        print("\n=== Training Classifier ===")
        classifier_model, classifier_meta = train_classifier(
            X_train, y_train_multi, X_val, y_val_multi,
            output_dir, n_classes=args.n_classes
        )
        print(f"Classifier model saved to {output_dir / 'lightgbm_classifier.txt'}")
    
    print("\n=== Training Complete ===")
    print(f"Models saved to: {output_dir}")


if __name__ == '__main__':
    main()