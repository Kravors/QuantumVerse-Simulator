import torch
ckpt = torch.load('models/geodesic_ode/best_model.pth', map_location='cpu')
sd = ckpt['model_state_dict']
print('Checkpoint val_loss:', ckpt['val_loss'])
print('Number of keys:', len(sd))
print('Keys:')
for k, v in sd.items():
    print(f'  {k}: {v.shape}')

# Determine architecture
linear_weights = [k for k in sd.keys() if 'weight' in k and 'network' in k]
print(f'\nLinear layers found at indices: {[k.split(".")[1] for k in linear_weights]}')
first_w = sd[linear_weights[0]]
last_w = sd[linear_weights[-1]]
print(f'First layer: {first_w.shape} (out_features, in_features)')
print(f'Last layer: {last_w.shape} (out_features, in_features)')
input_dim = first_w.shape[1]
hidden_dim = first_w.shape[0]
output_dim = last_w.shape[0]
print(f'\nDerived dimensions: input={input_dim}, hidden={hidden_dim}, output={output_dim}')
