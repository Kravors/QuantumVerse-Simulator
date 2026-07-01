import h5py
import numpy as np

with h5py.File('data/curvature_pilot_fixed.h5', 'r') as f:
    features = f['features'][:]

has_nan = np.isnan(features).any()
has_inf = np.isinf(features).any()
min_val = np.nanmin(features)
max_val = np.nanmax(features)
mean_val = np.nanmean(features)

print(f'Has NaN: {has_nan}')
print(f'Has Inf: {has_inf}')
print(f'Min: {min_val}')
print(f'Max: {max_val}')
print(f'Mean: {mean_val}')

# Also check per-feature NaN count
nan_per_feature = np.isnan(features).sum(axis=0)
print('NaN count per feature:', nan_per_feature)
