#include "CurvatureCalculator.h"
#include <stdexcept>

namespace quantumverse {

CurvatureCalculator::CurvatureCalculator(std::shared_ptr<MetricTensor> metric)
    : currentMetric_(std::move(metric))
    , fdStep_(1e-6)
    , singularityTolerance_(1e10)
    , ricciScalarCache_(0.0)
    , kretschmannCache_(0.0)
    , christoffelValid_(false)
    , riemannValid_(false)
    , ricciValid_(false) {
    if (!currentMetric_) {
        currentMetric_ = std::make_shared<MetricTensor>();
    }
    // Default field resolves the metric at the requested event so that
    // position-dependent metrics (e.g. SchwarzschildMetric) yield real
    // curvature instead of a single constant copy.
    metricField_ = [this](const Event4D& evt) -> MetricTensor {
        MetricTensor m;
        m.g = currentMetric_->evaluate(evt);
        return m;
    };
}

void CurvatureCalculator::setMetric(std::shared_ptr<MetricTensor> metric) {
    currentMetric_ = std::move(metric);
    christoffelValid_ = false;
    riemannValid_ = false;
    ricciValid_ = false;
}

void CurvatureCalculator::setMetricField(std::function<MetricTensor(const Event4D&)> field) {
    metricField_ = std::move(field);
    christoffelValid_ = false;
    riemannValid_ = false;
    ricciValid_ = false;
}

MetricTensor CurvatureCalculator::metricAt(const Event4D& position, int mu, double delta) const {
    Event4D perturbed = position;
    switch (mu) {
        case 0: perturbed.t += delta; break;
        case 1: perturbed.x += delta; break;
        case 2: perturbed.y += delta; break;
        case 3: perturbed.z += delta; break;
    }
    return metricField_(perturbed);
}

void CurvatureCalculator::computeChristoffel(const Event4D& position) const {
    if (christoffelValid_ && (position - lastEvalPosition_).spatialLength() < fdStep_ * 0.1) {
        return;
    }

    const double h = fdStep_;

    // Get metric at central position
    MetricTensor g0 = metricField_(position);
    MetricTensor inv = g0.inverse();

    // Reset cache
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                christoffelCache_[i][j][k] = 0.0;

    // Precompute central-difference derivatives of the metric in every
    // coordinate direction: deriv[dir][sigma][nu] = d_dir g_{sigma nu}.
    // Computing all four directions once (instead of re-deriving inside the
    // Christoffel assembly) keeps the index bookkeeping unambiguous.
    double deriv[4][4][4];
    for (int dir = 0; dir < 4; dir++) {
        MetricTensor g_plus = metricAt(position, dir, +h);
        MetricTensor g_minus = metricAt(position, dir, -h);
        for (int sigma = 0; sigma < 4; sigma++) {
            for (int nu = 0; nu < 4; nu++) {
                deriv[dir][sigma][nu] = (g_plus.g[sigma][nu] - g_minus.g[sigma][nu]) / (2.0 * h);
            }
        }
    }

    // Gamma^lambda_mu_nu = 0.5 * g^lambda_sigma * (d_mu g_sigma_nu + d_nu g_sigma_mu - d_sigma g_mu_nu)
    for (int mu = 0; mu < 4; mu++) {
        for (int nu = 0; nu < 4; nu++) {
            for (int lambda = 0; lambda < 4; lambda++) {
                double sum = 0.0;
                for (int sigma = 0; sigma < 4; sigma++) {
                    double d_mu_g_sigma_nu = deriv[mu][sigma][nu];
                    double d_nu_g_sigma_mu = deriv[nu][sigma][mu];
                    double d_sigma_g_mu_nu = deriv[sigma][mu][nu];
                    sum += inv.g[lambda][sigma] *
                           (d_mu_g_sigma_nu + d_nu_g_sigma_mu - d_sigma_g_mu_nu);
                }
                christoffelCache_[lambda][mu][nu] = 0.5 * sum;
            }
        }
    }

    lastEvalPosition_ = position;
    christoffelValid_ = true;
    riemannValid_ = false;
    ricciValid_ = false;
}

void CurvatureCalculator::computeRiemann(const Event4D& position) const {
    ensureChristoffel(position);

    if (riemannValid_) return;

    const double h = fdStep_;

    // R^rho_sigma_mu_nu = d_mu Gamma^rho_nu_sigma - d_nu Gamma^rho_mu_sigma
    //                     + Gamma^rho_mu_lambda * Gamma^lambda_nu_sigma
    //                     - Gamma^rho_nu_lambda * Gamma^lambda_mu_sigma
    //
    // The derivatives d_mu Gamma and d_nu Gamma depend only on positions
    // perturbed along mu and nu respectively (independent of rho,sigma), so we
    // evaluate Christoffel at the four perturbed positions ONCE per (mu,nu)
    // pair (64 evaluations total) instead of once per (rho,sigma,mu,nu) (1024
    // evaluations). The result is bit-for-bit equivalent but ~16x cheaper,
    // which keeps curvature-heavy discovery loops within the CI test timeout.

    // Cache the central Christoffel symbols for the product terms.
    std::array<std::array<std::array<double, 4>, 4>, 4> central = christoffelCache_;

    for (int mu = 0; mu < 4; mu++) {
        for (int nu = 0; nu < 4; nu++) {
            Event4D pos_p_mu = position;
            Event4D pos_m_mu = position;
            Event4D pos_p_nu = position;
            Event4D pos_m_nu = position;
            switch (mu) {
                case 0: pos_p_mu.t += h; pos_m_mu.t -= h; break;
                case 1: pos_p_mu.x += h; pos_m_mu.x -= h; break;
                case 2: pos_p_mu.y += h; pos_m_mu.y -= h; break;
                case 3: pos_p_mu.z += h; pos_m_mu.z -= h; break;
            }
            switch (nu) {
                case 0: pos_p_nu.t += h; pos_m_nu.t -= h; break;
                case 1: pos_p_nu.x += h; pos_m_nu.x -= h; break;
                case 2: pos_p_nu.y += h; pos_m_nu.y -= h; break;
                case 3: pos_p_nu.z += h; pos_m_nu.z -= h; break;
            }

            computeChristoffel(pos_p_mu);
            std::array<std::array<std::array<double, 4>, 4>, 4> Gamma_p_mu = christoffelCache_;
            computeChristoffel(pos_m_mu);
            std::array<std::array<std::array<double, 4>, 4>, 4> Gamma_m_mu = christoffelCache_;
            computeChristoffel(pos_p_nu);
            std::array<std::array<std::array<double, 4>, 4>, 4> Gamma_p_nu = christoffelCache_;
            computeChristoffel(pos_m_nu);
            std::array<std::array<std::array<double, 4>, 4>, 4> Gamma_m_nu = christoffelCache_;

            // Restore the central Christoffel symbols for the product terms.
            christoffelCache_ = central;

            for (int rho = 0; rho < 4; rho++) {
                for (int sigma = 0; sigma < 4; sigma++) {
                    double d_mu_Gamma = (Gamma_p_mu[rho][nu][sigma] - Gamma_m_mu[rho][nu][sigma]) / (2.0 * h);
                    double d_nu_Gamma = (Gamma_p_nu[rho][mu][sigma] - Gamma_m_nu[rho][mu][sigma]) / (2.0 * h);

                    double product1 = 0.0;  // Gamma^rho_mu_lambda * Gamma^lambda_nu_sigma
                    double product2 = 0.0;  // Gamma^rho_nu_lambda * Gamma^lambda_mu_sigma
                    for (int lambda = 0; lambda < 4; lambda++) {
                        product1 += central[rho][mu][lambda] * central[lambda][nu][sigma];
                        product2 += central[rho][nu][lambda] * central[lambda][mu][sigma];
                    }

                    riemannCache_[rho][sigma][mu][nu] = d_mu_Gamma - d_nu_Gamma + product1 - product2;
                }
            }
        }
    }

    riemannValid_ = true;
    ricciValid_ = false;
}

void CurvatureCalculator::computeRicci(const Event4D& position) const {
    ensureRiemann(position);

    if (ricciValid_) return;

    // R_mu_nu = g^rho_sigma * R_sigma_rho_mu_nu  (contract first and third indices of Riemann)
    // Equivalently: R_mu_nu = R^rho_mu_rho_nu
    MetricTensor inv = metricField_(position).inverse();

    for (int mu = 0; mu < 4; mu++) {
        for (int nu = 0; nu < 4; nu++) {
            double sum = 0.0;
            for (int rho = 0; rho < 4; rho++) {
                for (int sigma = 0; sigma < 4; sigma++) {
                    sum += inv.g[rho][sigma] * riemannCache_[sigma][rho][mu][nu];
                }
            }
            ricciCache_[mu][nu] = sum;
        }
    }

    ricciValid_ = true;
}

void CurvatureCalculator::computeRicciScalar(const Event4D& position) const {
    ensureRicci(position);

    MetricTensor inv = metricField_(position).inverse();
    double sum = 0.0;
    for (int mu = 0; mu < 4; mu++) {
        for (int nu = 0; nu < 4; nu++) {
            sum += inv.g[mu][nu] * ricciCache_[mu][nu];
        }
    }
    ricciScalarCache_ = sum;
}

void CurvatureCalculator::computeKretschmann(const Event4D& position) const {
    ensureRiemann(position);

    // K = R^{rho sigma mu nu} R_{rho sigma mu nu}
    // Lower the first index with the metric: R_{rho sigma mu nu} = g_{rho alpha} R^alpha_sigma_mu_nu
    MetricTensor metric = metricField_(position);
    MetricTensor inv = metric.inverse();
    std::array<std::array<std::array<std::array<double, 4>, 4>, 4>, 4> R_lowered;

    for (int rho = 0; rho < 4; rho++) {
        for (int sigma = 0; sigma < 4; sigma++) {
            for (int mu = 0; mu < 4; mu++) {
                for (int nu = 0; nu < 4; nu++) {
                    double sum = 0.0;
                    for (int alpha = 0; alpha < 4; alpha++) {
                        sum += metric.g[rho][alpha] * riemannCache_[alpha][sigma][mu][nu];
                    }
                    R_lowered[rho][sigma][mu][nu] = sum;
                }
            }
        }
    }

    // Fully contract the Riemann tensor: raise the remaining three indices
    // (sigma, mu, nu) with the inverse metric so that K = R^{rho sigma mu nu} R_{rho sigma mu nu}.
    // Contracting only the first index (R^rho_sigma_mu_nu * R_rho_sigma_mu_nu) is NOT the
    // Kretschmann scalar: it omits the g^{sigma beta} g^{mu gamma} g^{nu delta} factors and
    // is wrong for any metric whose inverse is not the identity (e.g. spherical coordinates).
    double sum = 0.0;
    for (int rho = 0; rho < 4; rho++) {
        for (int sigma = 0; sigma < 4; sigma++) {
            for (int mu = 0; mu < 4; mu++) {
                for (int nu = 0; nu < 4; nu++) {
                    double R_raised = 0.0;
                    for (int beta = 0; beta < 4; beta++) {
                        for (int gamma = 0; gamma < 4; gamma++) {
                            for (int delta = 0; delta < 4; delta++) {
                                R_raised += inv.g[sigma][beta] * inv.g[mu][gamma] * inv.g[nu][delta]
                                            * riemannCache_[rho][beta][gamma][delta];
                            }
                        }
                    }
                    sum += R_raised * R_lowered[rho][sigma][mu][nu];
                }
            }
        }
    }
    kretschmannCache_ = sum;
}

CurvatureResult CurvatureCalculator::computeAll(const Event4D& position) const {
    computeKretschmann(position);
    computeRicci(position);
    computeRicciScalar(position);

    CurvatureResult result;
    result.riemann = riemannCache_;
    result.ricci = ricciCache_;
    result.ricciScalar = ricciScalarCache_;
    result.kretschmann = kretschmannCache_;
    result.maxRiemannComponent = 0.0;

    for (int rho = 0; rho < 4; rho++) {
        for (int sigma = 0; sigma < 4; sigma++) {
            for (int mu = 0; mu < 4; mu++) {
                for (int nu = 0; nu < 4; nu++) {
                    double absVal = std::abs(riemannCache_[rho][sigma][mu][nu]);
                    if (absVal > result.maxRiemannComponent) {
                        result.maxRiemannComponent = absVal;
                    }
                }
            }
        }
    }

    // Compute Einstein tensor: G_mu_nu = R_mu_nu - 0.5 * g_mu_nu * R
    MetricTensor metric = metricField_(position);
    for (int mu = 0; mu < 4; mu++) {
        for (int nu = 0; nu < 4; nu++) {
            result.einstein[mu][nu] = ricciCache_[mu][nu] - 0.5 * metric.g[mu][nu] * ricciScalarCache_;
        }
    }

    // Compute Weyl tensor squared (simplified: K - 2*R^2/3 + R^2/12 for vacuum)
    // Full computation requires the decomposition; use vacuum approximation here
    result.weylSquared = kretschmannCache_ - 2.0 * ricciScalarCache_ * ricciScalarCache_ / 3.0
                         + ricciScalarCache_ * ricciScalarCache_ / 12.0;

    result.nearSingularity = (kretschmannCache_ > singularityTolerance_);

    return result;
}

const std::array<std::array<std::array<double, 4>, 4>, 4>& CurvatureCalculator::getChristoffel() const {
    return christoffelCache_;
}

const std::array<std::array<std::array<std::array<double, 4>, 4>, 4>, 4>& CurvatureCalculator::getRiemann() const {
    return riemannCache_;
}

const std::array<std::array<double, 4>, 4>& CurvatureCalculator::getRicci() const {
    return ricciCache_;
}

double CurvatureCalculator::getRicciScalar() const {
    return ricciScalarCache_;
}

double CurvatureCalculator::getKretschmann() const {
    return kretschmannCache_;
}

void CurvatureCalculator::setFDStep(double h) {
    fdStep_ = h;
}

void CurvatureCalculator::setSingularityTolerance(double tol) {
    singularityTolerance_ = tol;
}

bool CurvatureCalculator::isNearSingularity(const Event4D& position) const {
    computeKretschmann(position);
    return kretschmannCache_ > singularityTolerance_;
}

void CurvatureCalculator::ensureChristoffel(const Event4D& position) const {
    if (!christoffelValid_) {
        computeChristoffel(position);
    }
}

void CurvatureCalculator::ensureRiemann(const Event4D& position) const {
    ensureChristoffel(position);
    if (!riemannValid_) {
        computeRiemann(position);
    }
}

void CurvatureCalculator::ensureRicci(const Event4D& position) const {
    ensureRiemann(position);
    if (!ricciValid_) {
        computeRicci(position);
    }
}

} // namespace quantumverse