"""
Bayesian Evidence Calculator for QuantumVerse

Provides tools to compute marginal likelihood (evidence) for theoretical models
given observational data, enabling Bayesian model comparison via Bayes factors.

Implementation uses nested sampling (dynesty) for robust evidence computation.
"""

import numpy as np
import scipy.stats
from typing import Callable, Dict, List, Optional, Tuple, Union
from dataclasses import dataclass, field
from enum import Enum
import json
import h5py


class PriorType(Enum):
    """Supported prior distributions for parameters."""
    UNIFORM = "uniform"
    LOG_UNIFORM = "log_uniform"
    NORMAL = "normal"
    HALF_NORMAL = "half_normal"


class LikelihoodModel(Enum):
    """Supported likelihood functions."""
    GAUSSIAN = "gaussian"
    POISSON = "poisson"
    CUSTOM = "custom"


@dataclass
class ParameterRange:
    """Defines the prior range and type for a parameter."""
    name: str
    min: float
    max: float
    prior_type: PriorType = PriorType.UNIFORM
    prior_mean: Optional[float] = None  # For NORMAL
    prior_std: Optional[float] = None   # For NORMAL

    def validate(self):
        """Validate parameter range."""
        if self.min >= self.max:
            raise ValueError(f"Parameter {self.name}: min must be < max")
        if self.prior_type == PriorType.NORMAL:
            if self.prior_mean is None or self.prior_std is None:
                raise ValueError(f"Parameter {self.name}: NORMAL prior requires mean and std")
        if self.prior_type in [PriorType.UNIFORM, PriorType.LOG_UNIFORM]:
            if self.min < 0 and self.prior_type == PriorType.LOG_UNIFORM:
                raise ValueError(f"Parameter {self.name}: LOG_UNIFORM requires min > 0")


@dataclass
class EvidenceResult:
    """Results from evidence computation."""
    logZ: float                    # Log evidence (natural log)
    logZ_error: float              # Uncertainty in logZ
    map_params: Dict[str, float]   # Maximum a posteriori parameters
    log_likelihood_at_map: float  # Log likelihood at MAP
    posterior_samples: Dict[str, np.ndarray]  # Samples from posterior (parameter name -> array)
    method: str                   # "NESTED_SAMPLING" or "MCMC"
    n_samples: int                # Number of posterior samples
    runtime_seconds: float        # Computation time
    diagnostics: Dict = field(default_factory=dict)  # Additional diagnostics


class BayesianEvidenceCalculator:
    """
    Computes Bayesian evidence for a theory given data.

    Evidence Z = ∫ p(data|θ) p(θ) dθ is the marginal likelihood, serving as
    the normalizing constant in Bayes' theorem. It enables model comparison
    via Bayes factors K = Z₁/Z₂.

    This implementation uses nested sampling (dynesty) for robust computation
    of Z and posterior samples simultaneously.
    """

    def __init__(self,
                 theory: Callable[[Dict[str, float]], np.ndarray],
                 data: np.ndarray,
                 data_errors: Optional[np.ndarray] = None):
        """
        Initialize calculator.

        Args:
            theory: Callable that takes parameter dict and returns predicted data array
            data: Observed data (1D array)
            data_errors: Optional 1σ uncertainties per data point (if None, assumed 1.0)
        """
        self.theory = theory
        self.data = np.asarray(data)
        self.data_errors = np.ones_like(self.data) if data_errors is None else np.asarray(data_errors)

        if self.data.shape != self.data_errors.shape:
            raise ValueError("data and data_errors must have same shape")

        self.prior_ranges: List[ParameterRange] = []
        self.likelihood_model: LikelihoodModel = LikelihoodModel.GAUSSIAN
        self.nested_sampling_kwargs: Dict = {
            'nlive': 1000,      # Number of live points
            'bound': 'multi',   # Bound handling
            'sample': 'auto',   # Sampling method
            'maxiter': 10000,   # Maximum iterations
        }

    def set_prior_ranges(self, ranges: List[ParameterRange]):
        """Set parameter prior ranges."""
        for r in ranges:
            r.validate()
        self.prior_ranges = ranges

    def set_likelihood_model(self, model: LikelihoodModel):
        """Set likelihood function."""
        self.likelihood_model = model

    def set_nested_sampling_params(self, nlive: int = 1000, maxiter: int = 10000,
                                    bound: str = 'multi', sample: str = 'auto'):
        """Configure nested sampling parameters."""
        self.nested_sampling_kwargs.update({
            'nlive': nlive,
            'maxiter': maxiter,
            'bound': bound,
            'sample': sample
        })

    def _log_prior(self, params: Dict[str, float]) -> float:
        """Compute log prior probability."""
        logp = 0.0
        for pr in self.prior_ranges:
            val = params.get(pr.name)
            if val is None:
                raise ValueError(f"Parameter {pr.name} not provided")

            if pr.prior_type == PriorType.UNIFORM:
                if not (pr.min <= val <= pr.max):
                    return -np.inf
                logp -= np.log(pr.max - pr.min)
            elif pr.prior_type == PriorType.LOG_UNIFORM:
                if not (pr.min <= val <= pr.max):
                    return -np.inf
                logp -= np.log(val)  # dθ/θ for log-uniform
            elif pr.prior_type == PriorType.NORMAL:
                logp += scipy.stats.norm.logpdf(val, loc=pr.prior_mean, scale=pr.prior_std)
            elif pr.prior_type == PriorType.HALF_NORMAL:
                if val < 0:
                    return -np.inf
                logp += scipy.stats.halfnorm.logpdf(val, loc=0, scale=pr.prior_std)
            else:
                raise ValueError(f"Unknown prior type: {pr.prior_type}")

        return logp

    def _log_likelihood(self, params: Dict[str, float]) -> float:
        """Compute log likelihood p(data|params)."""
        # Get theory prediction
        try:
            model_pred = self.theory(params)
        except Exception as e:
            return -np.inf  # Theory failed, reject

        # Check shape
        if model_pred.shape != self.data.shape:
            raise ValueError(f"Theory output shape {model_pred.shape} != data shape {self.data.shape}")

        # Compute residuals
        residuals = (self.data - model_pred) / self.data_errors

        # Likelihood based on model
        if self.likelihood_model == LikelihoodModel.GAUSSIAN:
            # Gaussian: -0.5 * Σ (residual² + log(2πσ²))
            ll = -0.5 * np.sum(residuals**2 + np.log(2 * np.pi * self.data_errors**2))
        elif self.likelihood_model == LikelihoodModel.POISSON:
            # Poisson: Σ (data * log(model) - model - log(data!))
            # Using Stirling approximation for large counts: log(data!) ≈ data log data - data
            with np.errstate(divide='ignore', invalid='ignore'):
                term1 = self.data * np.log(model_pred)
                term2 = -model_pred
                term3 = -self.data * np.log(self.data) + self.data  # Stirling
                ll = np.sum(term1 + term2 + term3)
            ll = np.nan_to_num(ll, nan=-np.inf)
        else:
            raise ValueError(f"Likelihood model {self.likelihood_model} not implemented")

        return ll

    def _log_posterior(self, theta: np.ndarray) -> float:
        """Combined log posterior (prior + likelihood) for use with dynesty."""
        # Convert theta array to dict
        params = {pr.name: theta[i] for i, pr in enumerate(self.prior_ranges)}

        # Check prior
        logp = self._log_prior(params)
        if not np.isfinite(logp):
            return -np.inf

        # Likelihood
        logl = self._log_likelihood(params)
        if not np.isfinite(logl):
            return -np.inf

        return logp + logl

    def _prior_transform(self, u: np.ndarray) -> np.ndarray:
        """
        Transform from unit cube [0,1]^N to prior volume.
        Required by dynesty.
        """
        theta = np.zeros_like(u)
        for i, pr in enumerate(self.prior_ranges):
            ui = u[i]
            if pr.prior_type == PriorType.UNIFORM:
                theta[i] = pr.min + ui * (pr.max - pr.min)
            elif pr.prior_type == PriorType.LOG_UNIFORM:
                log_min = np.log(pr.min)
                log_max = np.log(pr.max)
                theta[i] = np.exp(log_min + ui * (log_max - log_min))
            elif pr.prior_type == PriorType.NORMAL:
                # Use inverse CDF (ppf)
                theta[i] = scipy.stats.norm.ppf(ui, loc=pr.prior_mean, scale=pr.prior_std)
            elif pr.prior_type == PriorType.HALF_NORMAL:
                # Half-normal: use folded normal
                theta[i] = scipy.stats.halfnorm.ppf(ui, loc=0, scale=pr.prior_std)
            else:
                raise ValueError(f"Unknown prior type: {pr.prior_type}")
        return theta

    def compute(self, method: str = "nested_sampling") -> EvidenceResult:
        """
        Compute Bayesian evidence.

        Args:
            method: "nested_sampling" (recommended) or "mcmc"

        Returns:
            EvidenceResult with logZ, samples, etc.
        """
        import time
        from dynesty import NestedSampler, DynamicNestedSampler

        start_time = time.time()

        if method == "nested_sampling":
            # Use static nested sampler
            # dynesty 3.x API: NestedSampler(loglikelihood, prior_transform, ndim, nlive, ...)
            sampler = NestedSampler(
                loglikelihood=self._log_posterior,
                prior_transform=self._prior_transform,
                ndim=len(self.prior_ranges),
                nlive=self.nested_sampling_kwargs.get('nlive', 1000),
                bound=self.nested_sampling_kwargs.get('bound', 'multi'),
                sample=self.nested_sampling_kwargs.get('sample', 'auto')
            )
            sampler.run_nested(maxiter=self.nested_sampling_kwargs.get('maxiter', 10000))
            results = sampler.results

            # Extract results
            logZ = results.logz[-1]
            logZ_error = results.logzerr[-1]

            # Get weighted posterior samples
            weights = np.exp(results.logl - results.logz[-1])
            indices = np.random.choice(len(weights), size=len(weights), p=weights/weights.sum())
            samples = results.samples[indices]

            # Convert to dict
            posterior_dict = {}
            for i, pr in enumerate(self.prior_ranges):
                posterior_dict[pr.name] = samples[:, i]

            # Find MAP (maximum posterior)
            map_idx = np.argmax(results.logl)
            map_params = {pr.name: results.samples[map_idx, i] for i, pr in enumerate(self.prior_ranges)}
            log_likelihood_at_map = results.logl[map_idx]

            runtime = time.time() - start_time

            return EvidenceResult(
                logZ=logZ,
                logZ_error=logZ_error,
                map_params=map_params,
                log_likelihood_at_map=log_likelihood_at_map,
                posterior_samples=posterior_dict,
                method="NESTED_SAMPLING",
                n_samples=len(samples),
                runtime_seconds=runtime,
                diagnostics={
                    'niter': results.niter,  # dynesty 3.x: scalar int
                    'samples': len(samples),
                    'nlive': self.nested_sampling_kwargs['nlive']
                }
            )

        elif method == "mcmc":
            # Use PyMC3/PyMC for MCMC (requires pymc)
            try:
                import pymc as pm
            except ImportError:
                raise ImportError("PyMC3/PyMC required for MCMC method. Install with: pip install pymc")

            with pm.Model() as model:
                # Define priors
                params_dict = {}
                for pr in self.prior_ranges:
                    if pr.prior_type == PriorType.UNIFORM:
                        params_dict[pr.name] = pm.Uniform(pr.name, lower=pr.min, upper=pr.max)
                    elif pr.prior_type == PriorType.LOG_UNIFORM:
                        # Log-uniform: use pm.Lognormal with sigma=1 (flat in log)
                        # Actually pm.Lognormal is different; use transformed variable
                        # For simplicity, use Uniform on log scale
                        log_min, log_max = np.log(pr.min), np.log(pr.max)
                        log_param = pm.Uniform(f"log_{pr.name}", lower=log_min, upper=log_max)
                        params_dict[pr.name] = pm.Deterministic(pr.name, pm.math.exp(log_param))
                    elif pr.prior_type == PriorType.NORMAL:
                        params_dict[pr.name] = pm.Normal(pr.name, mu=pr.prior_mean, sigma=pr.prior_std)
                    elif pr.prior_type == PriorType.HALF_NORMAL:
                        params_dict[pr.name] = pm.HalfNormal(pr.name, sigma=pr.prior_std)
                    else:
                        raise ValueError(f"Unknown prior type: {pr.prior_type}")

                # Define likelihood
                if self.likelihood_model == LikelihoodModel.GAUSSIAN:
                    mu = pm.Deterministic("mu", self.theory(params_dict))
                    obs = pm.Normal("obs", mu=mu, sigma=self.data_errors, observed=self.data)
                elif self.likelihood_model == LikelihoodModel.POISSON:
                    mu = pm.Deterministic("mu", self.theory(params_dict))
                    obs = pm.Poisson("obs", mu=mu, observed=self.data)
                else:
                    raise ValueError(f"Likelihood model {self.likelihood_model} not supported in MCMC")

                # Sample
                trace = pm.sample(
                    draws=2000,
                    tune=1000,
                    chains=4,
                    return_inferencedata=True,
                    progressbar=True
                )

                # Compute evidence using WAIC or LOO (approximate)
                # For true evidence, need to use bridge sampling or nested sampling
                # Here we provide an approximation via PSIS-LOO
                try:
                    from arviz import waic, loo
                    waic_val = waic(trace)
                    logZ_approx = -0.5 * waic_val.waic  # Not exact, but indicative
                    logZ_error = np.nan
                except:
                    logZ_approx = np.nan
                    logZ_error = np.nan

                # Extract posterior samples
                posterior_dict = {}
                for pr in self.prior_ranges:
                    posterior_dict[pr.name] = trace.posterior[pr.name].values.flatten()

                # MAP
                map_idx = np.argmax(trace.sample_stats.log_likelihood.sum(dim='chain').max(dim='draw').values)
                map_params = {pr.name: trace.posterior[pr.name].values.flatten()[map_idx] for pr in self.prior_ranges}
                log_likelihood_at_map = float(trace.sample_stats.log_likelihood.sum(dim='chain').max(dim='draw').values[map_idx])

                runtime = time.time() - start_time

                return EvidenceResult(
                    logZ=logZ_approx,
                    logZ_error=logZ_error,
                    map_params=map_params,
                    log_likelihood_at_map=log_likelihood_at_map,
                    posterior_samples=posterior_dict,
                    method="MCMC",
                    n_samples=len(posterior_dict[self.prior_ranges[0].name]),
                    runtime_seconds=runtime,
                    diagnostics={
                        'n_draws': 2000,
                        'n_tune': 1000,
                        'n_chains': 4
                    }
                )
        else:
            raise ValueError(f"Unknown method: {method}. Use 'nested_sampling' or 'mcmc'")

    def save_results(self, result: EvidenceResult, filename: str):
        """Save evidence results to HDF5."""
        with h5py.File(filename, 'w') as f:
            f.attrs['logZ'] = result.logZ
            f.attrs['logZ_error'] = result.logZ_error
            f.attrs['method'] = result.method
            f.attrs['n_samples'] = result.n_samples
            f.attrs['runtime_seconds'] = result.runtime_seconds

            # Save MAP params
            map_group = f.create_group('map_params')
            for k, v in result.map_params.items():
                map_group.attrs[k] = v

            # Save posterior samples
            samples_group = f.create_group('posterior_samples')
            for param_name, samples in result.posterior_samples.items():
                samples_group.create_dataset(param_name, data=samples)

            # Save diagnostics
            diag_group = f.create_group('diagnostics')
            for k, v in result.diagnostics.items():
                diag_group.attrs[k] = v

    @staticmethod
    def load_results(filename: str) -> EvidenceResult:
        """Load evidence results from HDF5."""
        with h5py.File(filename, 'r') as f:
            result = EvidenceResult(
                logZ=float(f.attrs['logZ']),
                logZ_error=float(f.attrs['logZ_error']),
                map_params=dict(f['map_params'].attrs),
                log_likelihood_at_map=float(f.attrs.get('log_likelihood_at_map', np.nan)),
                posterior_samples={k: v[:] for k, v in f['posterior_samples'].items()},
                method=str(f.attrs['method']),
                n_samples=int(f.attrs['n_samples']),
                runtime_seconds=float(f.attrs['runtime_seconds']),
                diagnostics=dict(f['diagnostics'].attrs)
            )
        return result


def bayes_factor(result1: EvidenceResult, result2: EvidenceResult) -> Tuple[float, str]:
    """
    Compute Bayes factor K = Z1 / Z2 and interpret strength of evidence.

    Returns:
        (logK, interpretation) where interpretation is from Jeffreys' scale
    """
    logK = result1.logZ - result2.logZ

    if logK < 0:
        direction = f"Model 2 favored by {abs(logK):.2f} log units"
    else:
        direction = f"Model 1 favored by {logK:.2f} log units"

    if abs(logK) < 1:
        interpretation = "Not worth more than a bare mention"
    elif abs(logK) < 2.5:
        interpretation = "Substantial evidence"
    elif abs(logK) < 5:
        interpretation = "Strong evidence"
    else:
        interpretation = "Decisive evidence"

    return logK, f"{direction} ({interpretation})"


# ========== Example Usage ==========

def example_linear_regression():
    """
    Test on simple linear regression: y = a*x + b + noise
    Compare models with/without intercept (b=0).
    """
    print("=" * 60)
    print("Example 1: Linear Regression Evidence Comparison")
    print("=" * 60)

    # Generate synthetic data
    np.random.seed(42)
    x = np.linspace(0, 10, 50)
    true_a, true_b = 2.5, 1.0
    sigma = 0.5
    y = true_a * x + true_b + np.random.normal(0, sigma, size=x.shape)

    # Model 1: y = a*x + b (2 parameters)
    def model1(params):
        a, b = params['a'], params['b']
        return a * x + b

    # Model 2: y = a*x (1 parameter, b=0)
    def model2(params):
        a = params['a']
        return a * x

    # Compute evidence for Model 1
    calc1 = BayesianEvidenceCalculator(
        theory=model1,
        data=y,
        data_errors=np.full_like(y, sigma)
    )
    calc1.set_prior_ranges([
        ParameterRange("a", 0, 5, PriorType.UNIFORM),
        ParameterRange("b", -2, 4, PriorType.UNIFORM)
    ])
    calc1.set_likelihood_model(LikelihoodModel.GAUSSIAN)
    result1 = calc1.compute(method="nested_sampling")

    # Compute evidence for Model 2
    calc2 = BayesianEvidenceCalculator(
        theory=model2,
        data=y,
        data_errors=np.full_like(y, sigma)
    )
    calc2.set_prior_ranges([
        ParameterRange("a", 0, 5, PriorType.UNIFORM)
    ])
    calc2.set_likelihood_model(LikelihoodModel.GAUSSIAN)
    result2 = calc2.compute(method="nested_sampling")

    # Compare
    logK, interpretation = bayes_factor(result1, result2)

    print(f"\nModel 1 (y = a*x + b):")
    print(f"  logZ = {result1.logZ:.2f} ± {result1.logZ_error:.2f}")
    print(f"  MAP: a={result1.map_params['a']:.3f}, b={result1.map_params['b']:.3f}")
    print(f"\nModel 2 (y = a*x):")
    print(f"  logZ = {result2.logZ:.2f} ± {result2.logZ_error:.2f}")
    print(f"  MAP: a={result2.map_params['a']:.3f}")
    print(f"\nBayes factor K = exp({logK:.2f})")
    print(f"  {interpretation}")
    print(f"\nTrue values: a={true_a}, b={true_b}")
    print("=" * 60)


def example_schwarzschild_mass_fit():
    """
    Fit Schwarzschild black hole mass to light deflection data.
    Compare GR (Schwarzschild) vs. exotic f(R) theory.
    """
    print("\n" + "=" * 60)
    print("Example 2: Schwarzschild Mass Fit")
    print("=" * 60)

    # Generate synthetic light deflection data
    # α = 4M/b (in geometric units, G=c=1)
    np.random.seed(42)
    M_true = 1.0  # True mass
    impact_params = np.array([2.0, 3.0, 5.0, 10.0, 20.0])
    true_deflection = 4 * M_true / impact_params
    sigma_angle = 0.01  # Measurement error
    observed_deflection = true_deflection + np.random.normal(0, sigma_angle, size=impact_params.shape)

    # Theory: deflection angle as function of M
    def gr_schwarzschild(params):
        M = params['M']
        return 4 * M / impact_params

    # Exotic theory: α = 4M/b * (1 + α1 * M/b)
    def exotic_fR(params):
        M = params['M']
        alpha1 = params['alpha1']
        return 4 * M / impact_params * (1 + alpha1 * M / impact_params)

    # Compute evidence for GR (1 parameter)
    calc_gr = BayesianEvidenceCalculator(
        theory=gr_schwarzschild,
        data=observed_deflection,
        data_errors=np.full_like(observed_deflection, sigma_angle)
    )
    calc_gr.set_prior_ranges([
        ParameterRange("M", 0.5, 2.0, PriorType.UNIFORM)
    ])
    calc_gr.set_likelihood_model(LikelihoodModel.GAUSSIAN)
    result_gr = calc_gr.compute(method="nested_sampling")

    # Compute evidence for exotic theory (2 parameters)
    calc_exotic = BayesianEvidenceCalculator(
        theory=exotic_fR,
        data=observed_deflection,
        data_errors=np.full_like(observed_deflection, sigma_angle)
    )
    calc_exotic.set_prior_ranges([
        ParameterRange("M", 0.5, 2.0, PriorType.UNIFORM),
        ParameterRange("alpha1", -1.0, 1.0, PriorType.UNIFORM)
    ])
    calc_exotic.set_likelihood_model(LikelihoodModel.GAUSSIAN)
    result_exotic = calc_exotic.compute(method="nested_sampling")

    # Compare
    logK, interpretation = bayes_factor(result_gr, result_exotic)

    print(f"\nGR (Schwarzschild):")
    print(f"  logZ = {result_gr.logZ:.2f} ± {result_gr.logZ_error:.2f}")
    print(f"  MAP: M={result_gr.map_params['M']:.4f}")
    print(f"\nExotic f(R):")
    print(f"  logZ = {result_exotic.logZ:.2f} ± {result_exotic.logZ_error:.2f}")
    print(f"  MAP: M={result_exotic.map_params['M']:.4f}, alpha1={result_exotic.map_params['alpha1']:.4f}")
    print(f"\nBayes factor K = exp({logK:.2f})")
    print(f"  {interpretation}")
    print(f"\nTrue M = {M_true}, alpha1 = 0 (GR)")
    print("=" * 60)


if __name__ == "__main__":
    # Run examples
    example_linear_regression()
    example_schwarzschild_mass_fit()