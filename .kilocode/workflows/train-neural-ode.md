
---

### 4. `.kilocode/workflows/train-neural-ode.md`

```markdown
# Workflow: train-neural-ode

Manage the Neural ODE surrogate training pipeline for Phase 2 Task 2.1.
Covers dataset generation, training, monitoring, and ONNX export.

## Step 1: Verify Python Environment

```bash
cd f:/syyyy/python
pip install -r requirements_task2_1.txt
python -c "import torch; print('PyTorch', torch.__version__); print('CUDA:', torch.cuda.is_available())"