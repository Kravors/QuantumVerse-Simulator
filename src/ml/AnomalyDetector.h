/**
 * @file AnomalyDetector.h
 * @brief Lightweight autoencoder for ML-based anomaly detection
 *
 * Implements a small feedforward autoencoder (20-10-2-10-20) using
 * plain std::vector matrix operations. No external ML runtime required.
 * Models are exported as JSON from the Python training script.
 */

#ifndef QUANTUMVERSE_ANOMALY_DETECTOR_H
#define QUANTUMVERSE_ANOMALY_DETECTOR_H

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include "ml/FeatureVector.h"

namespace quantumverse {

/**
 * @brief Lightweight autoencoder anomaly detector
 *
 * Architecture:
 *   Encoder: 20 -> 10 (ReLU) -> 2 (linear)
 *   Decoder: 2 -> 10 (ReLU) -> 20 (linear)
 *
 * Anomaly score = MSE(x, reconstruct(x)) over the 20-dim feature vector.
 * Higher scores indicate more anomalous states.
 *
 * Model format (JSON):
 * {
 *   "feature_dim": 20,
 *   "threshold": 0.05,
 *   "encoder": [
 *     {"in": 20, "out": 10, "weights": [[...], ...], "bias": [...]},
 *     {"in": 10, "out": 2,  "weights": [[...], ...], "bias": [...]}
 *   ],
 *   "decoder": [
 *     {"in": 2,  "out": 10, "weights": [[...], ...], "bias": [...]},
 *     {"in": 10, "out": 20, "weights": [[...], ...], "bias": [...]}
 *   ]
 * }
 */
class AnomalyDetector {
public:
    struct Layer {
        int in = 0;
        int out = 0;
        std::vector<std::vector<double>> weights;
        std::vector<double> bias;
    };

    struct Metadata {
        int feature_dim = 20;
        double threshold = 0.05;
        std::vector<double> mean;
        std::vector<double> std;
        std::vector<Layer> encoder;
        std::vector<Layer> decoder;
    };

    /**
     * @brief Default constructor (empty model, returns 0.0 score)
     */
    AnomalyDetector();

    /**
     * @brief Load model from JSON file
     * @param path Path to model JSON
     * @return true if loaded successfully
     */
    bool load(const std::string& path);

    /**
     * @brief Load model from JSON string
     * @param json_str JSON content
     * @return true if loaded successfully
     */
    bool loadFromString(const std::string& json_str);

    /**
     * @brief Check if model is loaded and ready
     */
    bool isLoaded() const noexcept { return loaded_; }

    /**
     * @brief Compute anomaly score for a feature vector
     *
     * @param features 20-dimensional feature vector
     * @return Reconstruction MSE (higher = more anomalous)
     *
     * @throws std::logic_error if model not loaded or dimension mismatch
     */
    double score(const FeatureVector& features) const;

    /**
     * @brief Classify a feature vector as anomalous or normal
     *
     * @param features Input features
     * @return true if score exceeds configured threshold
     */
    bool isAnomalous(const FeatureVector& features) const;

    /**
     * @brief Get configured detection threshold
     */
    double threshold() const noexcept { return metadata_.threshold; }

    /**
     * @brief Set detection threshold
     */
    void setThreshold(double t) { metadata_.threshold = t; }

    /**
     * @brief Get model metadata
     */
    const Metadata& getMetadata() const noexcept { return metadata_; }

private:
    Metadata metadata_;
    bool loaded_ = false;

    /**
     * @brief Forward pass through a single layer
     */
    static std::vector<double> forward(
        const std::vector<double>& input,
        const Layer& layer,
        bool relu
    );

    /**
     * @brief Compute MSE between two vectors
     */
    static double mse(const std::vector<double>& a, const std::vector<double>& b);

    /**
     * @brief Normalize features using stored mean/std
     */
    static std::vector<double> normalize(const std::vector<double>& raw, const std::vector<double>& mean, const std::vector<double>& std);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_ANOMALY_DETECTOR_H
