#ifndef QUANTUMVERSE_THEORY_SURROGATE_H
#define QUANTUMVERSE_THEORY_SURROGATE_H

#include <vector>
#include <memory>
#include <utility>
#include <cmath>
#include <algorithm>
#include <limits>

namespace quantumverse {
namespace discovery {

/**
 * @brief Lightweight Gaussian-process-like surrogate for theory reward prediction.
 *
 * Uses kernel ridge regression with an RBF kernel to provide:
 *   - Mean prediction for untried parameter points
 *   - Uncertainty estimate (predictive standard deviation)
 *
 * No external dependencies; fixed kernel hyperparameters (L=1.0, sigma_f=1.0,
 * noise lambda=1e-3). Suitable for active-learning point selection inside
 * TheoryDiscoveryAgent.
 */
class TheorySurrogate {
public:
    using DataPoint = std::pair<std::vector<double>, double>;

    TheorySurrogate();
    explicit TheorySurrogate(double length_scale, double sigma_f, double noise_lambda);

    void addTrainingPoint(const std::vector<double>& params, double reward);
    void setTrainingData(const std::vector<DataPoint>& data);

    /**
     * @brief Predict mean reward and standard deviation at a query point.
     * @param x Query parameter vector (normalized).
     * @param out_mean Predicted mean reward.
     * @param out_std Predicted standard deviation (uncertainty).
     */
    void predict(const std::vector<double>& x, double& out_mean, double& out_std) const;

    /**
     * @brief Predictive variance only (faster when mean is not needed).
     */
    double predictiveVariance(const std::vector<double>& x) const;

    size_t getTrainingSize() const { return training_data_.size(); }
    void clear() { training_data_.clear(); K_.clear(); alpha_.clear(); cached_ = false; }

private:
    std::vector<DataPoint> training_data_;
    mutable std::vector<std::vector<double>> K_;      // cached kernel matrix
    mutable std::vector<double> alpha_;               // cached (K + lambda I)^{-1} y

    double length_scale_;
    double sigma_f_;
    double noise_lambda_;

    mutable bool cached_;

    static double rbfKernel(const std::vector<double>& a,
                            const std::vector<double>& b,
                            double length_scale,
                            double sigma_f);

    void rebuildCache() const;
};

} // namespace discovery
} // namespace quantumverse

#endif // QUANTUMVERSE_THEORY_SURROGATE_H
