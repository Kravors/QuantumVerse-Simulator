#ifndef QUANTUMVERSE_CURVATURE_NORMALIZING_FLOW_H
#define QUANTUMVERSE_CURVATURE_NORMALIZING_FLOW_H

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

namespace quantumverse {

/**
 * @brief Normalizing flow model for curvature-based anomaly detection.
 * 
 * This class wraps a trained normalizing flow (RealNVP/MAF) exported to ONNX.
 * It computes anomaly scores from curvature invariant statistics:
 *   score = -log p(x)  (higher = more anomalous)
 * 
 * The model is trained on "normal" GR spacetimes (Schwarzschild, Kerr, RN, KN)
 * and detects exotic metrics (regular black holes, quantum-corrected) as anomalies.
 * 
 * Features: 15-dimensional summary statistics of curvature invariants:
 *   - Mean, std, min, max of Kretschmann scalar K
 *   - Percentiles (5%, 25%, 50%, 75%, 95%)
 *   - Skewness, kurtosis
 *   - Log-mean, log-std (log10 K)
 *   - Peakiness (max/mean), RMS
 * 
 * Dependencies: ONNX Runtime C++ library (optional, stub fallback available)
 * 
 * @note Thread-safe: predict() is const
 * @ingroup ml
 */
class CurvatureNormalizingFlow {
public:
    /**
     * @brief Model metadata (read-only after loading)
     */
    struct Metadata {
        int feature_dim;          ///< Input feature dimension (typically 15)
        std::string model_type;   ///< "realnvp" or "maf"
        int n_layers;             ///< Number of coupling layers
        int hidden_dim;           ///< Hidden layer size in coupling nets
        std::vector<double> mean; ///< Feature normalization mean
        std::vector<double> std;  ///< Feature normalization std
        bool is_loaded;           ///< True if ONNX model loaded successfully
    };

    /**
     * @brief Default constructor (creates stub model)
     * 
     * The stub returns a constant anomaly score (0.0 = normal).
     * Use loadONNXModel() to load a trained model.
     */
    CurvatureNormalizingFlow();

    /**
     * @brief Destructor
     */
    ~CurvatureNormalizingFlow();

    // Move semantics (non-copyable)
    CurvatureNormalizingFlow(const CurvatureNormalizingFlow&) = delete;
    CurvatureNormalizingFlow& operator=(const CurvatureNormalizingFlow&) = delete;
    CurvatureNormalizingFlow(CurvatureNormalizingFlow&&) noexcept;
    CurvatureNormalizingFlow& operator=(CurvatureNormalizingFlow&&) noexcept;

    /**
     * @brief Load ONNX model from file.
     * 
     * @param model_path Path to .onnx file (e.g., "models/curvature_flow/curvature_flow.onnx")
     * @return true if loaded successfully, false if ONNX unavailable or file invalid
     * 
     * @note If ONNX Runtime is not available, falls back to stub automatically.
     * @note Validates input/output dimensions against metadata.
     */
    bool loadONNXModel(const std::string& model_path);

    /**
     * @brief Check if ONNX model is loaded and ready for inference.
     * @return true if model loaded, false if using stub
     */
    bool isLoaded() const noexcept { return metadata_.is_loaded; }

    /**
     * @brief Get model metadata (read-only).
     * @return const reference to Metadata struct
     */
    const Metadata& getMetadata() const noexcept { return metadata_; }

    /**
     * @brief Compute anomaly score from curvature feature vector.
     * 
     * Computes: score = -log p(x) where p(x) is the learned probability density.
     * Higher scores indicate more anomalous (unlikely) curvature patterns.
     * 
     * @param features 15-dimensional feature vector (normalized using stored mean/std)
     * @return Anomaly score (double). If model not loaded, returns 0.0 (stub).
     * 
     * @throws std::invalid_argument if features.size() != feature_dim()
     * 
     * @note Features are automatically normalized internally before inference.
     * @note Thread-safe (const method)
     */
    double predict(const std::vector<double>& features) const;

    /**
     * @brief Compute anomaly score from raw (unnormalized) features.
     * 
     * Convenience overload that takes raw features and normalizes them
     * using the stored mean/std from training.
     * 
     * @param raw_features Unnormalized feature vector
     * @return Anomaly score (double)
     * 
     * @note Equivalent to: normalize(raw) → predict(normalized)
     */
    double predictRaw(const std::vector<double>& raw_features) const;

    /**
     * @brief Get expected feature dimension.
     * @return Number of features required (typically 15)
     */
    int featureDim() const noexcept { return metadata_.feature_dim; }

    /**
     * @brief Create default (stub) instance.
     * @return Unique pointer to new CurvatureNormalizingFlow
     */
    static std::unique_ptr<CurvatureNormalizingFlow> createDefault();

private:
    // PIMPL: opaque implementation details
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

    // Metadata (cached)
    Metadata metadata_;

    // Helper: normalize features
    std::vector<double> normalize(const std::vector<double>& raw) const;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_CURVATURE_NORMALIZING_FLOW_H
