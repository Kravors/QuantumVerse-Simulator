# Task 2.8 Setup Guide: Bayesian Evidence Calculator

**Plan Reference**: `PHASE2_AI_ACCELERATION_PLAN.md`, Task 2.8  
**Objective**: Compute Bayesian evidence Z = ∫ p(data|θ) p(θ) dθ for theory ranking  
**Duration**: 2 weeks  
**Status**: Ready to start (2026-05-04)

---

## 1. Task Overview

Bayesian evidence (marginal likelihood) quantifies how well a theory explains the data, automatically balancing:
- **Fit**: Likelihood p(data|θ) from simulation residuals
- **Complexity**: Prior volume p(θ) (Occam's razor)
- **Uncertainty**: Integrates over all parameter values

**Key Output**: Bayes factor K = Z₁/Z₂ for comparing two theories.

**Integration**: Used by RL Discovery Agent (Task 2.2) to rank discovered hypotheses and select the most promising ones for further exploration.

---

## 2. Mathematical Foundation

### Bayes' Theorem
```
p(θ|data) = p(data|θ) p(θ) / p(data)
```

Where:
- p(θ|data): Posterior (probability of parameters given data)
- p(data|θ): Likelihood (how well theory with parameters θ fits data)
- p(θ): Prior (initial belief about parameters)
- p(data): **Evidence** (marginal likelihood, normalizing constant)

### Evidence Integral
```
Z = p(data) = ∫ p(data|θ) p(θ) dθ
```

For a theory with N parameters, this is a high-dimensional integral. We'll compute it numerically using:
- **MCMC** (Markov Chain Monte Carlo) - e.g., PyMC3/NUTS
- **Nested Sampling** - e.g., dynesty (better for computing Z directly)

### Bayes Factor
```
K = Z₁ / Z₂
```

Interpretation (Jeffreys' scale):
- ln K < 1: Not worth more than a bare mention
- 1 ≤ ln K ≤ 2.5: Substantial evidence
- 2.5 ≤ ln K ≤ 5: Strong evidence
- ln K > 5: Decisive evidence

---

## 3. Architecture Design

### 3.1 BayesianEvidenceCalculator Class

**Purpose**: Compute evidence for a given theory + data.

**Interface**:
```cpp
class BayesianEvidenceCalculator {
public:
    struct EvidenceResult {
        double logZ;              // log evidence (for numerical stability)
        double logZ_error;        // uncertainty in logZ
        std::vector<double> best_params;  // MAP (maximum a posteriori) parameters
        double likelihood_at_best;       // p(data|θ_MAP)
        std::string method;       // "MCMC" or "NESTED_SAMPLING"
    };

    BayesianEvidenceCalculator(std::shared_ptr<TheoryPlugin> theory,
                               std::shared_ptr<DataAdapter> data);
    ~BayesianEvidenceCalculator();

    void setPrior(const std::vector<ParameterRange>& ranges);
    void setLikelihoodModel(LikelihoodModel model);  // Gaussian, Poisson, etc.
    void setSampler(SamplerType type, int n_samples, int n_walkers);

    EvidenceResult computeEvidence();
    std::vector<Sample> getSamples() const;  // posterior samples
    double computeBayesFactor(const EvidenceResult& other) const;
};
```

**Implementation Strategy**:
- C++ wrapper around Python Bayesian inference library (via pybind11)
- Or pure C++ implementation using existing libraries (e.g., libbi, StanMath)
- Given the complexity, likely **Python-based** with C++ interface (similar to Task 2.1)

### 3.2 TheoryComparator Class

**Purpose**: Compare multiple theories and rank them.

**Interface**:
```cpp
class TheoryComparator {
public:
    struct TheoryRank {
        std::string theory_name;
        double logZ;
        double logZ_error;
        double bayes_factor_vs_best;  // ln(K_i / K_best)
        std::string evidence_category;  // "weak", "substantial", "strong", "decisive"
    };

    void addTheory(const std::string& name, BayesianEvidenceCalculator& calc);
    std::vector<TheoryRank> rankTheories();
    std::string getBestTheory() const;
    double getEvidenceRatio(const std::string& t1, const std::string& t2) const;
};
```

### 3.3 Integration with DiscoveryEngine

**Changes to DiscoveryEngine**:
```cpp
class DiscoveryEngine {
private:
    std::unique_ptr<BayesianEvidenceCalculator> evidenceCalculator_;
    TheoryComparator comparator_;

public:
    void setEvidenceCalculator(std::unique_ptr<BayesianEvidenceCalculator> calc);
    EvidenceResult computeEvidenceForHypothesis(Hypothesis& hyp);
    std::vector<TheoryComparator::TheoryRank> rankTheories();
    void setPriorRanges(const std::vector<ParameterRange>& ranges);
};
```

**Workflow**:
1. RL agent proposes hypothesis (theory + parameters)
2. DiscoveryEngine runs simulation to get predicted data
3. Compare with observational data → compute likelihood
4. BayesianEvidenceCalculator integrates over parameter space → Z
5. TheoryComparator ranks all accumulated hypotheses
6. Best theories get promoted to "discovery" status

---

## 4. Implementation Plan

### Phase 1: Python Prototype (Week 1)

**Goal**: Validate Bayesian evidence computation on simple test cases.

**Files to Create**:
- `python/bayesian_evidence.py` - Core evidence calculator
- `python/test_evidence.py` - Unit tests (linear regression, Schwarzschild M-fit)
- `python/requirements_task2_8.txt` - Dependencies (pymc3, arviz, dynesty, numpy, scipy)

**Test Cases**:
1. **Linear regression** y = ax + b with Gaussian noise → analytic Z available
2. **Schwarzschild mass fit** to light deflection data → compare GR vs. exotic f(R) theory
3. **Regular black hole** vs. Schwarzschild → Bayes factor should favor Schwarzschild if data is GR-like

**Implementation**:
- Use **dynesty** (nested sampling) for robust evidence computation
- Implement likelihood models: Gaussian, Poisson, custom
- Handle parameter priors: uniform, log-uniform, normal
- Output: logZ, samples, best-fit parameters

### Phase 2: C++ Interface (Week 1-2)

**Goal**: Expose evidence calculator to C++ via pybind11 or pure C++ library.

**Option A: Python Bridge** (easier, consistent with Task 2.1/2.5)
- Create `src/bayesian/BayesianEvidenceCalculator.h/cpp`
- Use pybind11 to call Python `bayesian_evidence.py` from C++
- Embed Python interpreter in QuantumVerse (already done for Task 2.1)
- Serialize data via JSON or HDF5

**Option B: Pure C++** (harder, more dependencies)
- Use **StanMath** or **libbi** for MCMC/nested sampling
- No Python dependency, but steeper learning curve
- Better performance, but longer development time

**Recommendation**: Start with Option A (Python bridge) for rapid prototyping, then optimize later if needed.

### Phase 3: Integration with RL Agent (Week 2)

**Goal**: RL agent uses evidence to rank and select theories.

**Changes to RLDiscoveryAgent**:
- Add `computeEvidence()` method that calls `BayesianEvidenceCalculator`
- Modify reward function to include evidence bonus: `r += w_evidence * (logZ - logZ_baseline)`
- Store evidence in `Hypothesis` struct
- Use `TheoryComparator` to maintain leaderboard of top theories

**Testing**:
- Run RL agent with and without evidence bonus
- Verify that agent learns to prefer simpler theories (Occam's razor)
- Check that evidence correlates with physical plausibility

---

## 5. Dependencies

### Python
```
numpy>=1.24
scipy>=1.10
pymc3>=3.11  # or pymc>=4.0
arviz>=0.16   # for diagnostics
dynesty>=2.0  # nested sampling (preferred for evidence)
corner>=2.0   # plotting
matplotlib>=3.6
h5py>=3.8
```

### C++
- pybind11 (already used in Task 2.1)
- Python development headers (for embedding)
- JSON library (nlohmann/json already used)

---

## 6. API Design (C++)

**Header**: `src/bayesian/BayesianEvidenceCalculator.h`

```cpp
#pragma once
#include <memory>
#include <vector>
#include <string>
#include "spacetime/TheoryPlugin.h"
#include "data/DataAdapter.h"

namespace quantumverse {
namespace bayesian {

struct ParameterRange {
    std::string name;
    double min;
    double max;
    PriorType prior;  // UNIFORM, LOG_UNIFORM, NORMAL
    double prior_mean;  // for NORMAL
    double prior_std;   // for NORMAL
};

struct EvidenceResult {
    double logZ;              // log evidence
    double logZ_error;        // uncertainty
    std::vector<double> map_params;  // maximum a posteriori
    double log_likelihood_at_map;
    std::vector<std::vector<double>> posterior_samples;  // samples from posterior
    std::string method;       // "NESTED_SAMPLING" or "MCMC"
};

enum class LikelihoodModel {
    GAUSSIAN,       // p(data|θ) ∝ exp(-0.5 χ²)
    POISSON,        // count data
    CUSTOM          // user-defined log-likelihood
};

class BayesianEvidenceCalculator {
public:
    BayesianEvidenceCalculator(std::shared_ptr<TheoryPlugin> theory,
                               std::shared_ptr<DataAdapter> data);
    ~BayesianEvidenceCalculator();

    void setPriorRanges(const std::vector<ParameterRange>& ranges);
    void setLikelihoodModel(LikelihoodModel model);
    void setSampler(const std::string& method, int n_samples = 1000);

    EvidenceResult compute();
    double getLogEvidence() const { return result_.logZ; }
    const EvidenceResult& getResult() const { return result_; }

private:
    std::shared_ptr<TheoryPlugin> theory_;
    std::shared_ptr<DataAdapter> data_;
    std::vector<ParameterRange> prior_ranges_;
    LikelihoodModel likelihood_model_;
    std::string sampler_method_;
    int n_samples_;
    EvidenceResult result_;

    // Helper: call Python implementation
    EvidenceResult callPythonEvidence();
    // Helper: pure C++ fallback (simple grid integration for N≤3)
    EvidenceResult computeGridIntegration();
};

} // namespace bayesian
} // namespace quantumverse
```

---

## 7. Implementation Steps

### Step 1: Python Prototype (Day 1-3)
1. Set up Python environment with dynesty, pymc3, arviz
2. Implement `bayesian_evidence.py`:
   - `class EvidenceCalculator`: accepts theory, data, priors
   - `compute_nested_sampling()`: uses dynesty
   - `compute_mcmc()`: uses pymc3/NUTS
   - `gaussian_likelihood()`, `poisson_likelihood()`
   - `save_results()`: JSON + HDF5
3. Test on linear regression (compare to analytic Z)
4. Test on Schwarzschild mass fit (synthetic data)

### Step 2: C++ Wrapper (Day 4-7)
1. Create `src/bayesian/BayesianEvidenceCalculator.h` (interface)
2. Create `src/bayesian/BayesianEvidenceCalculator.cpp`:
   - Constructor stores theory/data shared_ptrs
   - `compute()` calls Python via pybind11 module
   - Fallback to `computeGridIntegration()` for simple cases (N≤3)
3. Add pybind11 module in `src/bayesian/bayesian_pybind.cpp`:
   - Expose `BayesianEvidenceCalculator` to Python
   - Or expose Python `EvidenceCalculator` to C++ (reverse direction)
4. Update `CMakeLists.txt` to add new source files

### Step 3: Integration (Day 8-10)
1. Modify `DiscoveryEngine.h/cpp`:
   - Add `BayesianEvidenceCalculator` member
   - Add `setEvidenceCalculator()`, `computeEvidenceForHypothesis()`
   - Add `TheoryComparator` for ranking
2. Modify `RLDiscoveryAgent.h/cpp`:
   - Add evidence to `Hypothesis` struct
   - Modify reward to include evidence bonus
   - Add `computeEvidence()` method
3. Update `examples/example_discovery.cpp` to demonstrate evidence ranking

### Step 4: Testing & Documentation (Day 11-14)
1. Write unit tests `tests/test_bayesian_evidence.cpp`:
   - Test evidence calculation on simple Gaussian
   - Test TheoryComparator ranking
   - Test integration with DiscoveryEngine
2. Write tutorial: "Using Bayesian Evidence to Rank Theories"
3. Update `PLAN7_PROGRESS_REPORT.md` with Task 2.8 status
4. Validate on real data: fit GW170817 with different theories

---

## 8. Success Criteria

- [ ] Python prototype computes correct evidence for linear regression (within 1% of analytic)
- [ ] C++ wrapper can call Python evidence calculator successfully
- [ ] Evidence calculator works on Schwarzschild mass fit (N=1, converges in <1 min)
- [ ] TheoryComparator correctly ranks theories by evidence
- [ ] RL agent uses evidence to select better theories (empirical improvement)
- [ ] All unit tests pass
- [ ] Documentation complete (API reference + tutorial)

---

## 9. Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Nested sampling too slow for real-time RL | High | Use cheap surrogate (e.g., Laplace approximation) for RL, full nested sampling offline |
| Python dependency increases deployment complexity | Medium | Provide pure C++ fallback (grid integration for N≤3) |
| Evidence sensitive to prior choice | High | Implement default weakly-informative priors, allow user override |
| High-dimensional integrals (N>10) numerically challenging | High | Start with low-N theories (Schwarzschild: N=1, Kerr: N=2), defer high-D to later |
| pybind11 embedding issues (GIL, threading) | Medium | Use subprocess instead of embedding; or use gRPC |

---

## 10. Next Immediate Actions

1. **Create Python environment** for Task 2.8:
   ```bash
   cd f:/syyyy
   python -m venv venv_task2_8
   venv_task2_8\Scripts\activate
   pip install -r python/requirements_task2_8.txt
   ```

2. **Implement Python prototype** (`python/bayesian_evidence.py`)

3. **Write unit tests** (`python/test_evidence.py`)

4. **Create C++ header** (`src/bayesian/BayesianEvidenceCalculator.h`)

5. **Update CMake** to add new source files

---

## 11. References

- **Nested Sampling**: Skilling (2004) - "Nested Sampling"
- **dynesty**: Speagle (2020) - "dynesty: A Dynamic Nested Sampling Package"
- **PyMC3**: Salvatier et al. (2016) - "Probabilistic programming in Python using PyMC3"
- **Bayesian Model Comparison**: Kass & Raftery (1995) - "Bayes Factors"

---

*End of TASK2_8_SETUP_GUIDE.md*
