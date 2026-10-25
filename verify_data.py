import h5py
import numpy as np

with h5py.File('data/flow_data.h5', 'r') as f:
    print('=== Counts ===')
    print(f'  healthy/train: {len(f["healthy/train"])} samples')
    print(f'  healthy/val: {len(f["healthy/val"])} samples')
    print(f'  healthy/test: {len(f["healthy/test"])} samples')
    for cat in ['near_horizon_spike', 'naked_singularity', 'signature_flip', 'ricci_excursion']:
        print(f'  flow_anomaly/{cat}: {len(f[f"flow_anomaly/{cat}"])} samples')
    print(f'  probe_valid/nan_injection: {len(f["probe_valid/nan_injection"])} samples')
    print(f'  differential/mass_transition: {len(f["differential/mass_transition"])} sweeps')

    print()
    print('=== Metadata ===')
    meta = f['metadata']
    for key in meta.attrs:
        print(f'  {key}: {meta.attrs[key]}')

    print()
    print('=== Sample feature check ===')
    s = f['healthy/train/sample_000000']
    print(f'  features shape: {s["features"].shape}')
    print(f'  probe_positions shape: {s["probe_positions"].shape}')
    print(f'  M={s.attrs["M"]:.1f}, a={s.attrs["a"]:.2f}, Q={s.attrs["Q"]:.2f}, family={s.attrs["family"]}')

    feat = s['features']
    valid_mask = feat[:, 4] == 1.0
    cont = feat[valid_mask, 0:4]
    print(f'  Valid points: {valid_mask.sum()}/{len(feat)}')
    print(f'  All finite: {np.all(np.isfinite(cont))}')

    for i in range(5):
        s2 = f[f'healthy/train/sample_{i:06d}']
        if s2.attrs['family'] == 'schwarzschild':
            print(f'  Schwarzschild sample {i}: M={s2.attrs["M"]:.1f}, K_resid range=[{feat[:,0].min():.3f}, {feat[:,0].max():.3f}]')
            break
