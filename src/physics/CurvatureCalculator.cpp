#include "CurvatureCalculator.h"
#include <stdexcept>

namespace quantumverse {

CurvatureCalculator::CurvatureCalculator(std::shared_ptr<MetricTensor> metric)
    : currentMetric_(std::move(metric))
    , fdStep_(1e-6)
    , singularityTolerance_(1e10)
    , christoffelValid_(false)
    , riemannValid_(false)
    , ricciValid_(false)
    , ricciScalarCache_(0.0)
    , kretschmannCache_(0.0) {
    if (!currentMetric_) {
        currentMetric_ = std::make_shared<MetricTensor>();
    }
    metricField_ = [this](const Event4D& evt) -> MetricTensor {
        return *currentMetric_;
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

    // For each derivative direction mu
    for (int mu = 0; mu < 4; mu++) {
        // Central difference: [g(x+h) - g(x-h)] / (2h)
        MetricTensor g_plus = metricAt(position, mu, +h);
        MetricTensor g_minus = metricAt(position, mu, -h);

        // dg[sigma][nu] = d_mu g_sigma_nu
        double dg[4][4];
        for (int sigma = 0; sigma < 4; sigma++) {
            for (int nu = 0; nu < 4; nu++) {
                dg[sigma][nu] = (g_plus.g[sigma][nu] - g_minus.g[sigma][nu]) / (2.0 * h);
            }
        }

        // Gamma^lambda_mu_nu = 0.5 * g^lambda_sigma * (d_mu g_sigma_nu + d_nu g_sigma_mu - d_sigma g_mu_nu)
        for (int lambda = 0; lambda < 4; lambda++) {
            for (int nu = 0; nu < 4; nu++) {
                double sum = 0.0;
                for (int sigma = 0; sigma < 4; sigma++) {
                    double dg_mu_sigma_nu = dg[sigma][nu];
                    double dg_nu_sigma_mu = dg[sigma][mu];

                    // Need d_sigma g_mu_nu
                    double dg_sigma_mu_nu;
                    if (sigma == mu) {
                        dg_sigma_mu_nu = dg[mu][nu];
                    } else {
                        MetricTensor gp = metricAt(position, sigma, +h);
                        MetricTensor gm = metricAt(position, sigma, -h);
                        dg_sigma_mu_nu = (gp.g[mu][nu] - gm.g[mu][nu]) / (2.0 * h);
                    }

                    sum += inv.g[lambda][sigma] *
                           (dg_mu_sigma_nu + dg_nu_sigma_mu - dg_sigma_mu_nu);
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

    for (int rho = 0; rho < 4; rho++) {
        for (int sigma = 0; sigma < 4; sigma++) {
            for (int mu = 0; mu < 4; mu++) {
                for (int nu = 0; nu < 4; nu++) {
                    // Numerical derivatives of Christoffel symbols
                    // d_mu Gamma^rho_nu_sigma
                    Event4D pos_p_mu = position;
                    Event4D pos_m_mu = position;
                    switch (mu) {
                        case 0: pos_p_mu.t += h; pos_m_mu.t -= h; break;
                        case 1: pos_p_mu.x += h; pos_m_mu.x -= h; break;
                        case 2: pos_p_mu.y += h; pos_m_mu.y -= h; break;
                        case 3: pos_p_mu.z += h; pos_m_mu.z -= h; break;
                    }
                    computeChristoffel(pos_p_mu);
                    double Gamma_rho_nu_sigma_mu = christoffelCache_[rho][nu][sigma];

                    computeChristoffel(pos_m_mu);
                    double Gamma_rho_nu_sigma_mu_m = christoffelCache_[rho][nu][sigma];

                    double d_mu_Gamma = (Gamma_rho_nu_sigma_mu - Gamma_rho_nu_sigma_mu_m) / (2.0 * h);

                    // d_nu Gamma^rho_mu_sigma
                    Event4D pos_p_nu = position;
                    Event4D pos_m_nu = position;
                    switch (nu) {
                        case 0: pos_p_nu.t += h; pos_m_nu.t -= h; break;
                        case 1: pos_p_nu.x += h; pos_m_nu.x -= h; break;
                        case 2: pos_p_nu.y += h; pos_m_nu.y -= h; break;
                        case 3: pos_p_nu.z += h; pos_m_nu.z -= h; break;
                    }
                    computeChristoffel(pos_p_nu);
                    double Gamma_rho_mu_sigma_nu = christoffelCache_[rho][mu][sigma];

                    computeChristoffel(pos_m_nu);
                    double Gamma_rho_mu_sigma_nu_m = christoffelCache_[rho][mu][sigma];

                    double d_nu_Gamma = (Gamma_rho_mu_sigma_nu - Gamma_rho_mu_sigma_nu_m) / (2.0 * h);

                    // Christoffel product terms at the central position
                    ensureChristoffel(position);
                    double product1 = 0.0;  // Gamma^rho_mu_lambda * Gamma^lambda_nu_sigma
                    double product2 = 0.0;  // Gamma^rho_nu_lambda * Gamma^lambda_mu_sigma
                    for (int lambda = 0; lambda < 4; lambda++) {
                        product1 += christoffelCache_[rho][mu][lambda] * christoffelCache_[lambda][nu][sigma];
                        product2 += christoffelCache_[rho][nu][lambda] * christoffelCache_[lambda][mu][sigma];
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

    // K = R^rho_sigma_mu_nu * R_rho_sigma_mu_nu
    // Need to lower all indices: K = g_rho_alpha g_sigma_beta g_mu_gamma g_nu_delta * R^alpha_beta_gamma_delta * R^rho_sigma_mu_nu
    // Simpler: K = R_{rho sigma mu nu} * R^{rho sigma mu nu}
    // We compute by contracting with the metric

    MetricTensor metric = metricField_(position);
    MetricTensor inv = metric.inverse();

    double sum = 0.0;
    for (int rho = 0; rho < 4; rho++) {
        for (int sigma = 0; sigma < 4; sigma++) {
            for (int mu = 0; mu < 4; mu++) {
                for (int nu = 0; nu < 4; nu++) {
                    // Lower all indices of Riemann
                    double R_lowered = 0.0;
                    for (int a = 0; a < 4; a++) {
                        for (int b = 0; b < 4; b++) {
                            for (int c = 0; c < 4; c++) {
                                for (int d = 0; d < 4; d++) {
                                    R_lowered += metric.g[rho][a] * metric.g[sigma][b] *
                                                 metric.g[mu][c] * metric.g[nu][d] *
                                                 riemannCache_[a][b][c][d];
                                }
                            }
                        }
                    }
                    sum += R_lowered * riemannCache_[rho][sigma][mu][nu];
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