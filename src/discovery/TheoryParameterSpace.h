#ifndef QUANTUMVERSE_THEORY_PARAMETER_SPACE_H
#define QUANTUMVERSE_THEORY_PARAMETER_SPACE_H

#include <vector>
#include <string>
#include <map>
#include <utility>

namespace quantumverse {
namespace discovery {

/**
 * @brief Represents a single theory parameter with bounds and metadata.
 */
struct TheoryParameter {
    std::string name;
    double min;
    double max;
    double default_value;
    std::string description;
    std::string unit;

    TheoryParameter(std::string n, double lo, double hi, double def = 0.0,
                    std::string desc = "", std::string u = "")
        : name(std::move(n)), min(lo), max(hi), default_value(def),
          description(std::move(desc)), unit(std::move(u)) {}
};

/**
 * @brief Defines the search space for autonomous theory discovery.
 *
 * Encodes parameter ranges for supported modified-gravity theories:
 *   - f(R): power-law exponent n and coupling alpha
 *   - Brans-Dicke: omega and phi0
 *   - LQG: Immirzi parameter gamma and polymer scale lambda
 */
class TheoryParameterSpace {
public:
    enum class TheoryType {
        FR_GRAVITY,
        BRANS_DICKE,
        LOOP_QUANTUM_GRAVITY,
        CUSTOM
    };

    explicit TheoryParameterSpace(TheoryType type = TheoryType::FR_GRAVITY);

    std::vector<TheoryParameter> getParameters() const;
    int getParameterDimension() const;
    std::map<std::string, double> parameterVectorToMap(
        const std::vector<double>& vec
    ) const;
    std::vector<double> mapToParameterVector(
        const std::map<std::string, double>& map
    ) const;
    std::string getTheoryName() const;
    TheoryType getTheoryType() const;

private:
    TheoryType type_;
    std::vector<TheoryParameter> parameters_;
    std::vector<std::string> param_names_;
};

} // namespace discovery
} // namespace quantumverse

#endif // QUANTUMVERSE_THEORY_PARAMETER_SPACE_H
