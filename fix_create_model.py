#!/usr/bin/env python3
import re

with open('python/train_curvature_flow_fixed.py', 'r', encoding='utf-8') as f:
    content = f.read()

old_func = '''def create_model(feature_dim, model_type='realnvp', n_layers=4, hidden_dim=128):
    """Create normalizing flow model."""
    if model_type.lower() == 'realnvp':
        if HAS_NFLOWS:
            # Use nflows library
            transform = transforms.CompositeTransform([
                transforms.AffineCouplingTransform(
                    mask=torch.arange(feature_dim) % 2 == 0,  # alternate mask
                    transform_net=lambda d: nn.Sequential(
                        nn.Linear(d // 2, hidden_dim),
                        nn.ReLU(),
                        nn.Linear(hidden_dim, hidden_dim),
                        nn.ReLU(),
                        nn.Linear(hidden_dim, d)  # outputs scale and shift
                    )
                ) for _ in range(n_layers)
            ])
            flow = flows.Flow(transform, distributions.StandardNormal([feature_dim]))
        else:
            # Use simple implementation
            print("Using simple RealNVP implementation (nflows not available)")
            flow = SimpleRealNVP(feature_dim, n_layers=n_layers, hidden_dim=hidden_dim)
    else:
        raise ValueError(f"Unsupported model type: {model_type}")
    
    return flow'''

new_func = '''def create_model(feature_dim, model_type='realnvp', n_layers=4, hidden_dim=128):
    """Create normalizing flow model."""
    if model_type.lower() == 'realnvp':
        if HAS_NFLOWS:
            # Use nflows library
            # nflows expects transform_net_create_fn(num_identity, num_transform_output)
            transform_list = []
            for i in range(n_layers):
                # Alternating mask
                mask = torch.zeros(feature_dim)
                if i % 2 == 0:
                    mask[::2] = 1.0
                else:
                    mask[1::2] = 1.0
                
                # Factory with correct signature
                def make_transform_net(num_identity, num_transform_output):
                    return nn.Sequential(
                        nn.Linear(num_identity, hidden_dim),
                        nn.ReLU(),
                        nn.Linear(hidden_dim, hidden_dim),
                        nn.ReLU(),
                        nn.Linear(hidden_dim, num_transform_output)
                    )
                
                transform_list.append(
                    transforms.AffineCouplingTransform(
                        mask=mask,
                        transform_net_create_fn=make_transform_net
                    )
                )
            transform = transforms.CompositeTransform(transform_list)
            flow = flows.Flow(transform, distributions.StandardNormal([feature_dim]))
        else:
            print("Using simple RealNVP implementation (nflows not available)")
            flow = SimpleRealNVP(feature_dim, n_layers=n_layers, hidden_dim=hidden_dim)
    else:
        raise ValueError(f"Unsupported model type: {model_type}")
    return flow'''

if old_func in content:
    content = content.replace(old_func, new_func)
    print("create_model function replaced successfully")
else:
    print("ERROR: Could not find exact create_model function")
    idx = content.find('def create_model')
    if idx >= 0:
        print(f"Found at position {idx}, snippet: {repr(content[idx:idx+300])}")
    else:
        print("Function not found at all")

with open('python/train_curvature_flow_fixed.py', 'w', encoding='utf-8') as f:
    f.write(content)

print("File written: python/train_curvature_flow_fixed.py")
