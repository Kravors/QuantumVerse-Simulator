"""Make the ONNX model batch dimension dynamic using onnx package."""
import onnx
import numpy as np
import os

model_path = "models/geodesic_ode/geodesic_ode.onnx"
out_path = "models/geodesic_ode/geodesic_ode_dynamic.onnx"

model = onnx.load(model_path)
inp = model.graph.input[0]
out = model.graph.output[0]

print(f"Input: {inp.name} shape={inp.type.tensor_type.shape}")
print(f"Output: {out.name} shape={out.type.tensor_type.shape}")

# Replace static batch dim with dynamic
inp.type.tensor_type.shape.dim[0].dim_param = "batch_size"
out.type.tensor_type.shape.dim[0].dim_param = "batch_size"

# Also handle any intermediate value_info that has fixed batch dim
for val in model.graph.value_info:
    if val.type.tensor_type.shape.dim:
        first_dim = val.type.tensor_type.shape.dim[0]
        if first_dim.dim_value == 1:
            first_dim.dim_param = "batch_size"

# Validate
try:
    onnx.checker.check_model(model)
    print("ONNX check passed")
except Exception as e:
    print(f"ONNX check warning: {e}")

# Save
onnx.save(model, out_path)
print(f"Saved to {out_path}")
print(f"Exists: {os.path.isfile(out_path)}")

# Test with different batch sizes
import onnxruntime as ort
sess = ort.InferenceSession(out_path, providers=['CPUExecutionProvider'])
for bs in [1, 4, 16, 64, 256, 1024]:
    test = np.random.randn(bs, 10).astype(np.float32)
    res = sess.run(['final_event'], {'input': test})
    print(f"  batch={bs}: output shape={res[0].shape} OK")
print("All dynamic batch tests passed!")