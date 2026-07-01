#!/usr/bin/env python3

with open('python/train_curvature_flow_fixed.py', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Find if __name__ line
idx = None
for i, line in enumerate(lines):
    if "if __name__ == '__main__':" in line:
        idx = i
        break

if idx is None:
    print("ERROR: Could not find if __name__ line")
    import sys; sys.exit(1)

print(f"Found if __name__ at line {idx+1}")

# Insert main function before it
main_lines = [
    'def main():\n',
    '    parser = argparse.ArgumentParser(description=\'Train normalizing flow for anomaly detection\')\n',
    '    parser.add_argument(\'--dataset\', type=str, default=\'data/curvature_dataset.h5\',\n',
    '                        help=\'Path to curvature dataset HDF5\')\n',
    '    parser.add_argument(\'--output-dir\', type=str, default=\'models/curvature_flow\',\n',
    '                        help=\'Output directory for model checkpoints\')\n',
    '    parser.add_argument(\'--model-type\', type=str, default=\'realnvp\', choices=[\'realnvp\', \'maf\'],\n',
    '                        help=\'Flow architecture\')\n',
    '    parser.add_argument(\'--n-layers\', type=int, default=4, help=\'Number of coupling layers\')\n',
    '    parser.add_argument(\'--hidden-dim\', type=int, default=128, help=\'Hidden dimension in coupling nets\')\n',
    '    parser.add_argument(\'--epochs\', type=int, default=200, help=\'Training epochs\')\n',
    '    parser.add_argument(\'--batch-size\', type=int, default=64, help=\'Batch size\')\n',
    '    parser.add_argument(\'--lr\', type=float, default=1e-3, help=\'Learning rate\')\n',
    '    parser.add_argument(\'--val-split\', type=float, default=0.2, help=\'Validation split\')\n',
    '    parser.add_argument(\'--seed\', type=int, default=42, help=\'Random seed\')\n',
    '    parser.add_argument(\'--device\', type=str, default=\'cuda\' if torch.cuda.is_available() else \'cpu\',\n',
    '                        help=\'Device (cuda or cpu)\')\n',
    '    args = parser.parse_args()\n',
    '    \n',
    '    # Set seed\n',
    '    torch.manual_seed(args.seed)\n',
    '    np.random.seed(args.seed)\n',
    '    \n',
    '    # Load dataset\n',
    '    features, labels, params = load_dataset(args.dataset)\n',
    '    feature_dim = features.shape[1]\n',
    '    \n',
    '    # Normalize features (standard scaling)\n',
    '    mean = features.mean(axis=0, keepdims=True)\n',
    '    std = features.std(axis=0, keepdims=True) + 1e-8\n',
    '    features_norm = (features - mean) / std\n',
    '    \n',
    '    # Save normalization stats\n',
    '    os.makedirs(args.output_dir, exist_ok=True)\n',
    '    np.savez(os.path.join(args.output_dir, \'normalization_stats.npz\'),\n',
    '             mean=mean, std=std)\n',
    '    \n',
    '    # Split train/val\n',
    '    n_val = int(len(features_norm) * args.val_split)\n',
    '    indices = np.random.permutation(len(features_norm))\n',
    '    train_indices = indices[n_val:]\n',
    '    val_indices = indices[:n_val]\n',
    '    \n',
    '    train_features = torch.tensor(features_norm[train_indices], dtype=torch.float32)\n',
    '    val_features = torch.tensor(features_norm[val_indices], dtype=torch.float32)\n',
    '    \n',
    '    train_dataset = TensorDataset(train_features)\n',
    '    val_dataset = TensorDataset(val_features)\n',
    '    \n',
    '    train_loader = DataLoader(train_dataset, batch_size=args.batch_size, shuffle=True)\n',
    '    val_loader = DataLoader(val_dataset, batch_size=args.batch_size, shuffle=False)\n',
    '    \n',
    '    print(f\"\\nTrain set: {len(train_features)} samples\")\n',
    '    print(f\"Val set: {len(val_features)} samples\")\n',
    '    print(f\"Feature dimension: {feature_dim}\")\n',
    '    print(f\"Device: {args.device}\")\n',
    '    print(f\"Model: {args.model_type} with {args.n_layers} layers, hidden_dim={args.hidden_dim}\")\n',
    '    \n',
    '    # Create model\n',
    '    model = create_model(feature_dim, model_type=args.model_type,\n',
    '                         n_layers=args.n_layers, hidden_dim=args.hidden_dim)\n',
    '    \n',
    '    # Count parameters\n',
    '    n_params = sum(p.numel() for p in model.parameters())\n',
    '    print(f\"Model parameters: {n_params:,}\")\n',
    '    \n',
    '    # Train\n',
    '    model, train_losses, val_losses = train_flow(\n',
    '        model, train_loader, val_loader,\n',
    '        n_epochs=args.epochs, lr=args.lr,\n',
    '        device=args.device, checkpoint_dir=args.output_dir\n',
    '    )\n',
    '    \n',
    '    # Save final normalization stats in model metadata\n',
    '    metadata = {\n',
    '        \'feature_dim\': feature_dim,\n',
    '        \'model_type\': args.model_type,\n',
    '        \'n_layers\': args.n_layers,\n',
    '        \'hidden_dim\': args.hidden_dim,\n',
    '        \'train_samples\': len(train_features),\n',
    '        \'val_samples\': len(val_features),\n',
    '        \'mean\': mean.tolist(),\n',
    '        \'std\': std.tolist(),\n',
    '    }\n',
    '    with open(os.path.join(args.output_dir, \'metadata.yaml\'), \'w\') as f:\n',
    '        yaml.dump(metadata, f)\n',
    '    \n',
    '    print(f\"\\nModel saved to {args.output_dir}\")\n',
    '    print(f\"  - best_model.pth (best checkpoint)\")\n',
    '    print(f\"  - final_model.pth (final epoch)\")\n',
    '    print(f\"  - normalization_stats.npz (feature scaling)\")\n',
    '    print(f\"  - metadata.yaml (model config)\")\n',
    '    print(f\"  - training_history.npz (loss curves)\")\n',
]

# Insert before if __name__
for line in reversed(main_lines):
    lines.insert(idx, line)

with open('python/train_curvature_flow_fixed.py', 'w', encoding='utf-8') as f:
    f.writelines(lines)

print(f"Inserted main() function. File now has {len(lines)} lines.")
