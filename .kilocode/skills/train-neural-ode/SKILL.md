
---

### 5. `.kilo/skills/train-neural-ode/SKILL.md`

```markdown
---
name: train-neural-ode
description: Manage the Neural ODE training pipeline—check environment, dataset, resume training, monitor progress, and export ONNX. Use when the user says "train", "neural ODE", "surrogate model", or "Phase 2.1".
metadata:
  project: QuantumVerse
  phase: 2
  task: 2.1
---

# Neural ODE Training

## When to Use
When the user wants to train, resume, or monitor the geodesic surrogate model.

## Process

### 1. Verify Environment
```bash
cd f:/syyyy/python
pip install -r requirements_task2_1.txt
python -c "import torch; print(torch.__version__, torch.cuda.is_available())"