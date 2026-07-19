#include "TheorySurrogate.h"
#include <stdexcept>
#include <numeric>
#include <random>

namespace quantumverse {
namespace discovery {

TheorySurrogate::TheorySurrogate()
    : length_scale_(1.0),
      sigma_f_(1.0),
      noise_lambda_(1e-3),
      cached_(false)
{
}

TheorySurrogate::TheorySurrogate(double length_scale, double sigma_f, double noise_lambda)
    : length_scale_(length_scale),
      sigma_f_(sigma_f),
      noise_lambda_(noise_lambda),
      cached_(false)
{
    if (length_scale <= 0.0) throw std::invalid_argument("length_scale must be positive");
    if (sigma_f <= 0.0) throw std::invalid_argument("sigma_f must be positive");
    if (noise_lambda < 0.0) throw std::invalid_argument("noise_lambda must be non-negative");
}

void TheorySurrogate::addTrainingPoint(const std::vector<double>& params, double reward) {
    training_data_.emplace_back(params, reward);
    cached_ = false;
}

void TheorySurrogate::setTrainingData(const std::vector<DataPoint>& data) {
    training_data_ = data;
    cached_ = false;
}

double TheorySurrogate::rbfKernel(const std::vector<double>& a,
                                   const std::vector<double>& b,
                                   double length_scale,
                                   double sigma_f) {
    if (a.size() != b.size()) return 0.0;
    double sq_dist = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        double d = a[i] - b[i];
        sq_dist += d * d;
    }
    return sigma_f * sigma_f * std::exp(-sq_dist / (2.0 * length_scale * length_scale));
}

void TheorySurrogate::rebuildCache() const {
    size_t n = training_data_.size();
    if (n == 0) {
        K_ = {};
        alpha_ = {};
        cached_ = true;
        return;
    }

    // Build kernel matrix K_{ij} = k(x_i, x_j)
    K_.assign(n, std::vector<double>(n, 0.0));
    for (size_t i = 0; i < n; ++i) {
        K_[i][i] = rbfKernel(training_data_[i].first, training_data_[i].first,
                             length_scale_, sigma_f_) + noise_lambda_;
        for (size_t j = i + 1; j < n; ++j) {
            double k = rbfKernel(training_data_[i].first, training_data_[j].first,
                                 length_scale_, sigma_f_);
            K_[i][j] = k;
            K_[j][i] = k;
        }
    }

    // Solve alpha = K^{-1} y using Cholesky-like approach
    // We use a simple LU-like forward/back substitution for small matrices
    // For numerical stability, add jitter if needed
    const double jitter = 1e-6;
    std::vector<std::vector<double>> A = K_;
    std::vector<double> y(n);
    for (size_t i = 0; i < n; ++i) {
        y[i] = training_data_[i].second;
        A[i][i] += jitter;
    }

    // Forward substitution for L (lower triangular)
    std::vector<std::vector<double>> L(n, std::vector<double>(n, 0.0));
    for (size_t i = 0; i < n; ++i) {
        L[i][i] = std::sqrt(A[i][i]);
        for (size_t j = i + 1; j < n; ++j) {
            double sum = A[j][i];
            for (size_t k = 0; k < i; ++k) {
                sum -= L[j][k] * L[i][k];
            }
            L[j][i] = sum / L[i][i];
        }
    }

    // Solve L * z = y
    std::vector<double> z(n, 0.0);
    for (size_t i = 0; i < n; ++i) {
        double sum = y[i];
        for (size_t j = 0; j < i; ++j) {
            sum -= L[i][j] * z[j];
        }
        z[i] = sum / L[i][i];
    }

    // Solve L^T * alpha = z
    alpha_.assign(n, 0.0);
    for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
        double sum = z[i];
        for (size_t j = i + 1; j < n; ++j) {
            sum -= L[j][i] * alpha_[j];
        }
        alpha_[i] = sum / L[i][i];
    }

    cached_ = true;
}

void TheorySurrogate::predict(const std::vector<double>& x, double& out_mean, double& out_std) const {
    if (training_data_.empty()) {
        out_mean = 0.0;
        out_std = 1.0;
        return;
    }

    if (!cached_) rebuildCache();

    size_t n = training_data_.size();
    std::vector<double> k_star(n);
    for (size_t i = 0; i < n; ++i) {
        k_star[i] = rbfKernel(x, training_data_[i].first, length_scale_, sigma_f_);
    }

    // mean = k*^T * alpha
    double mean = 0.0;
    for (size_t i = 0; i < n; ++i) {
        mean += k_star[i] * alpha_[i];
    }

    // variance = k(x,x) - k*^T * (K + lambda I)^{-1} * k*
    // We compute via Cholesky solve
    double k_xx = rbfKernel(x, x, length_scale_, sigma_f_);

    if (n == 1) {
        double k_inv = 1.0 / K_[0][0];
        double v = k_star[0] * k_inv;
        double var = k_xx - v * k_star[0];
        out_mean = mean;
        out_std = std::sqrt(std::max(0.0, var));
        return;
    }

    // Reuse L from cache - we need to solve (K + lambda I)^{-1} * k_star
    // L was built in rebuildCache; we need to extract it
    // Since K_ is stored but L isn't, let's just do another forward/back solve
    // For efficiency, we can use the fact that we already have K_
    
    // Simple approach: compute var = k_xx - sum_i sum_j k_i * (K^{-1})_{ij} * k_j
    // We already have alpha for the full solve, but we need k* inverse here
    // For simplicity, use a small-scale direct solve approach
    
    // Use the cached K_ and a simple solve
    std::vector<std::vector<double>> A = K_;
    std::vector<double> b = k_star;
    
    const double jitter = 1e-6;
    for (size_t i = 0; i < n; ++i) A[i][i] += jitter;

    // Cholesky
    std::vector<std::vector<double>> Lmat(n, std::vector<double>(n, 0.0));
    for (size_t i = 0; i < n; ++i) {
        Lmat[i][i] = std::sqrt(A[i][i]);
        for (size_t j = i + 1; j < n; ++j) {
            double sum = A[j][i];
            for (size_t k = 0; k < i; ++k) {
                sum -= Lmat[j][k] * Lmat[i][k];
            }
            Lmat[j][i] = sum / Lmat[i][i];
        }
    }

    // Solve L * z = b
    std::vector<double> z(n, 0.0);
    for (size_t i = 0; i < n; ++i) {
        double sum = b[i];
        for (size_t j = 0; j < i; ++j) {
            sum -= Lmat[i][j] * z[j];
        }
        z[i] = sum / Lmat[i][i];
    }

    // Solve L^T * v = z
    std::vector<double> v(n, 0.0);
    for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
        double sum = z[i];
        for (size_t j = i + 1; j < n; ++j) {
            sum -= Lmat[j][i] * v[j];
        }
        v[i] = sum / Lmat[i][i];
    }

    double kTv = 0.0;
    for (size_t i = 0; i < n; ++i) {
        kTv += k_star[i] * v[i];
    }

    double var = k_xx - kTv;
    out_mean = mean;
    out_std = std::sqrt(std::max(0.0, var));
}

double TheorySurrogate::predictiveVariance(const std::vector<double>& x) const {
    if (training_data_.empty()) return 1.0;
    if (!cached_) rebuildCache();

    size_t n = training_data_.size();
    std::vector<double> k_star(n);
    for (size_t i = 0; i < n; ++i) {
        k_star[i] = rbfKernel(x, training_data_[i].first, length_scale_, sigma_f_);
    }

    double k_xx = rbfKernel(x, x, length_scale_, sigma_f_);

    if (n == 1) {
        double k_inv = 1.0 / K_[0][0];
        double v = k_star[0] * k_inv;
        return std::max(0.0, k_xx - v * k_star[0]);
    }

    std::vector<std::vector<double>> A = K_;
    std::vector<double> b = k_star;
    const double jitter = 1e-6;
    for (size_t i = 0; i < n; ++i) A[i][i] += jitter;

    std::vector<std::vector<double>> Lmat(n, std::vector<double>(n, 0.0));
    for (size_t i = 0; i < n; ++i) {
        Lmat[i][i] = std::sqrt(A[i][i]);
        for (size_t j = i + 1; j < n; ++j) {
            double sum = A[j][i];
            for (size_t k = 0; k < i; ++k) {
                sum -= Lmat[j][k] * Lmat[i][k];
            }
            Lmat[j][i] = sum / Lmat[i][i];
        }
    }

    std::vector<double> z(n, 0.0);
    for (size_t i = 0; i < n; ++i) {
        double sum = b[i];
        for (size_t j = 0; j < i; ++j) {
            sum -= Lmat[i][j] * z[j];
        }
        z[i] = sum / Lmat[i][i];
    }

    std::vector<double> v(n, 0.0);
    for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
        double sum = z[i];
        for (size_t j = i + 1; j < n; ++j) {
            sum -= Lmat[j][i] * v[j];
        }
        v[i] = sum / Lmat[i][i];
    }

    double kTv = 0.0;
    for (size_t i = 0; i < n; ++i) {
        kTv += k_star[i] * v[i];
    }

    return std::max(0.0, k_xx - kTv);
}

} // namespace discovery
} // namespace quantumverse
