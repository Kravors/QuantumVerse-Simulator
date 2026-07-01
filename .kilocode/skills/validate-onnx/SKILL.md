
---

### 4. `.kilo/skills/validate-onnx/SKILL.md`

```markdown
---
name: validate-onnx
description: Inspect, test, benchmark, and optionally re-export the Neural ODE ONNX model. Use when the user asks about ONNX model validation, inference testing, or model export.
metadata:
  project: QuantumVerse
  phase: 2
  task: 2.1
---

# ONNX Model Validation

## When to Use
When the user asks to validate, test, benchmark, or re-export the ONNX model.

## Prerequisites
```bash
cd f:/syyyy
python -c "import onnx, onnxruntime, numpy, torch; print('OK')"