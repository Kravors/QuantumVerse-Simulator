#ifndef QUANTUMVERSE_WORLDLINE_H
#define QUANTUMVERSE_WORLDLINE_H

/**
 * @file WorldLine.h
 * @brief QuantumVerse Simulator - 4D World-Line with Proper Time Parametrisation
 *
 * Represents a timelike world-line in 4D spacetime, storing a sequence of
 * spacetime events parametrised by proper time τ. Supports interpolation,
 * length computation, and causal classification of segments.
 *
 * Based on: Spacetime.md - Worldlines and Proper Time
 */

#include "Event4D.h"
#include <vector>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <optional>

namespace quantumverse {

/**
 * @brief A single point on a world-line with proper time and four-velocity.
 */
struct WorldLinePoint {
    Event4D event;          ///< Spacetime event
    double properTime;      ///< Proper time τ along the world-line (seconds)
    std::array<double, 4> fourVelocity;  ///< Four-velocity u^μ = dx^μ/dτ

    WorldLinePoint()
        : event(), properTime(0.0), fourVelocity({0.0, 0.0, 0.0, 0.0}) {}

    WorldLinePoint(const Event4D& evt, double tau, const std::array<double, 4>& vel)
        : event(evt), properTime(tau), fourVelocity(vel) {}

    /**
     * @brief Compute the Lorentz factor γ = dt/dτ for this point.
     */
    double lorentzFactor() const {
        return fourVelocity[0];  // u^0 = γc in coordinate time parametrisation
    }

    /**
     * @brief Compute the spatial speed |v|/c at this point.
     */
    double spatialSpeed() const {
        double spatialNormSq = fourVelocity[1] * fourVelocity[1]
                             + fourVelocity[2] * fourVelocity[2]
                             + fourVelocity[3] * fourVelocity[3];
        double temporal = fourVelocity[0];
        if (std::abs(temporal) < 1e-15) return 0.0;
        return std::sqrt(spatialNormSq) / std::abs(temporal) * Event4D::C;
    }
};

/**
 * @brief A world-line: a curve x^μ(τ) in 4D spacetime parametrised by proper time.
 *
 * Stores a discrete sequence of WorldLinePoints and provides interpolation,
 * segment analysis, and causal classification.
 */
class WorldLine {
public:
    /**
     * @brief Construct an empty world-line.
     */
    WorldLine() = default;

    /**
     * @brief Construct a world-line from a geodesic trajectory.
     * @param geodesic Vector of GeodesicStep from GeodesicIntegrator
     */
    explicit WorldLine(const std::vector<GeodesicStep>& geodesic) {
        points_.reserve(geodesic.size());
        for (const auto& step : geodesic) {
            // Approximate four-velocity from consecutive steps
            std::array<double, 4> vel = {0, 0, 0, 0};
            if (points_.size() >= 1) {
                const auto& prev = points_.back();
                double dtau = step.properTime - prev.properTime;
                if (std::abs(dtau) > 1e-15) {
                    vel[0] = (step.event.t - prev.event.t) / dtau;
                    vel[1] = (step.event.x - prev.event.x) / dtau;
                    vel[2] = (step.event.y - prev.event.y) / dtau;
                    vel[3] = (step.event.z - prev.event.z) / dtau;
                }
            }
            points_.emplace_back(step.event, step.properTime, vel);
        }
        // Set final velocity to zero (endpoint)
        if (!points_.empty()) {
            points_.back().fourVelocity = {0, 0, 0, 0};
        }
    }

    /**
     * @brief Add a point to the world-line.
     */
    void addPoint(const WorldLinePoint& point) {
        points_.push_back(point);
    }

    /**
     * @brief Add a point with computed proper time from the metric.
     * @param event The spacetime event
     * @param fourVelocity Four-velocity at this event
     * @param metric Metric tensor for proper time computation
     */
    void addPoint(const Event4D& event,
                  const std::array<double, 4>& fourVelocity,
                  const MetricTensor& metric) {
        double tau = 0.0;
        if (!points_.empty()) {
            const auto& prev = points_.back();
            double ds2 = metric.interval(prev.event, event);
            if (ds2 < 0) {
                tau = prev.properTime + std::sqrt(-ds2) / Event4D::C;
            } else {
                tau = prev.properTime;  // Spacelike: don't advance proper time
            }
        }
        points_.emplace_back(event, tau, fourVelocity);
    }

    /**
     * @brief Get the number of points on the world-line.
     */
    size_t size() const { return points_.size(); }

    /**
     * @brief Check if the world-line is empty.
     */
    bool empty() const { return points_.empty(); }

    /**
     * @brief Access a point by index.
     */
    const WorldLinePoint& operator[](size_t i) const {
        if (i >= points_.size()) {
            throw std::out_of_range("WorldLine index out of range");
        }
        return points_[i];
    }

    /**
     * @brief Get the first point (initial event).
     */
    const WorldLinePoint& front() const {
        if (points_.empty()) throw std::runtime_error("WorldLine is empty");
        return points_.front();
    }

    /**
     * @brief Get the last point (final event).
     */
    const WorldLinePoint& back() const {
        if (points_.empty()) throw std::runtime_error("WorldLine is empty");
        return points_.back();
    }

    /**
     * @brief Get the total proper time elapsed along the world-line.
     * @return Total proper time in seconds.
     */
    double totalProperTime() const {
        if (points_.empty()) return 0.0;
        return back().properTime - front().properTime;
    }

    /**
     * @brief Get the total coordinate time elapsed.
     * @return Total coordinate time in seconds.
     */
    double totalCoordinateTime() const {
        if (points_.empty()) return 0.0;
        return back().event.t - front().event.t;
    }

    /**
     * @brief Compute the total spacetime length (proper time integral).
     * @param metric Metric tensor for interval computation.
     * @return Total proper time along the world-line.
     */
    double length(const MetricTensor& metric) const {
        if (points_.size() < 2) return 0.0;

        double total = 0.0;
        for (size_t i = 1; i < points_.size(); i++) {
            double ds2 = metric.interval(points_[i - 1].event, points_[i].event);
            if (ds2 < 0) {
                total += std::sqrt(-ds2) / Event4D::C;
            }
        }
        return total;
    }

    /**
     * @brief Interpolate the world-line at a given proper time using linear interpolation.
     * @param tau Target proper time.
     * @return Interpolated WorldLinePoint, or std::nullopt if outside range.
     */
    std::optional<WorldLinePoint> interpolate(double tau) const {
        if (points_.empty()) return std::nullopt;
        if (tau < front().properTime || tau > back().properTime) return std::nullopt;

        // Find the segment containing tau
        for (size_t i = 1; i < points_.size(); i++) {
            if (points_[i].properTime >= tau) {
                const auto& p0 = points_[i - 1];
                const auto& p1 = points_[i];

                double t0 = p0.properTime;
                double t1 = p1.properTime;
                double dt = t1 - t0;

                if (std::abs(dt) < 1e-15) return p0;

                double alpha = (tau - t0) / dt;

                WorldLinePoint result;
                result.properTime = tau;
                result.event.t = p0.event.t + alpha * (p1.event.t - p0.event.t);
                result.event.x = p0.event.x + alpha * (p1.event.x - p0.event.x);
                result.event.y = p0.event.y + alpha * (p1.event.y - p0.event.y);
                result.event.z = p0.event.z + alpha * (p1.event.z - p0.event.z);

                // Interpolate four-velocity
                for (int j = 0; j < 4; j++) {
                    result.fourVelocity[j] = p0.fourVelocity[j] + alpha * (p1.fourVelocity[j] - p0.fourVelocity[j]);
                }

                return result;
            }
        }

        return back();
    }

    /**
     * @brief Get all events on the world-line.
     */
    const std::vector<WorldLinePoint>& points() const { return points_; }

    /**
     * @brief Classify a segment between two points as timelike, lightlike, or spacelike.
     */
    static const char* classifySegment(const WorldLinePoint& a, const WorldLinePoint& b) {
        double dt = b.event.t - a.event.t;
        double dx = b.event.x - a.event.x;
        double dy = b.event.y - a.event.y;
        double dz = b.event.z - a.event.z;

        double ds2 = Event4D::C2 * dt * dt - dx * dx - dy * dy - dz * dz;

        if (ds2 > 1e-10) return "timelike";
        if (ds2 < -1e-10) return "spacelike";
        return "lightlike";
    }

    /**
     * @brief Clear all points from the world-line.
     */
    void clear() { points_.clear(); }

    /**
     * @brief Reserve space for N points.
     */
    void reserve(size_t n) { points_.reserve(n); }

    /**
     * @brief Get the spatial position at a given proper time (interpolated).
     */
    std::optional<std::array<double, 3>> positionAt(double tau) const {
        auto pt = interpolate(tau);
        if (!pt) return std::nullopt;
        return std::array<double, 3>{pt->event.x, pt->event.y, pt->event.z};
    }

    /**
     * @brief Get the spatial distance traveled along the world-line
     *        (integrated spatial path length, NOT proper time).
     */
    double spatialPathLength() const {
        if (points_.size() < 2) return 0.0;

        double total = 0.0;
        for (size_t i = 1; i < points_.size(); i++) {
            double dx = points_[i].event.x - points_[i - 1].event.x;
            double dy = points_[i].event.y - points_[i - 1].event.y;
            double dz = points_[i].event.z - points_[i - 1].event.z;
            total += std::sqrt(dx * dx + dy * dy + dz * dz);
        }
        return total;
    }

private:
    std::vector<WorldLinePoint> points_;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_WORLDLINE_H