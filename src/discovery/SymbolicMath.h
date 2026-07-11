#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <optional>

namespace quantumverse {

/**
 * @brief Symbolic mathematics interface for field equation generation.
 * 
 * This class provides a C++ interface to SymPy (via Python subprocess) for
 * symbolic tensor calculus, field equation derivation, and mathematical
 * expression manipulation. It supports:
 * - Einstein field equation computation
 * - Christoffel symbol symbolic derivation
 * - Curvature tensor symbolic computation
 * - Field equation simplification and validation
 * 
 * @ingroup discovery
 */
class SymbolicMath {
public:
    /**
     * @brief Result of symbolic computation.
     */
    struct SymbolicResult {
        std::string expression;
        bool is_valid = false;
        std::string error_message;
    };

    /**
     * @brief Tensor component specification.
     */
    struct TensorComponent {
        int upper_idx;               ///< Upper index (-1 if none)
        int lower_idx1;              ///< First lower index
        int lower_idx2;              ///< Second lower index (for Riemann)
        int lower_idx3;              ///< Third lower index (for Riemann)
        int lower_idx4;              ///< Fourth lower index (for Riemann)
    };

    SymbolicMath();
    ~SymbolicMath();

    // Non-copyable but movable
    SymbolicMath(const SymbolicMath&) = delete;
    SymbolicMath& operator=(const SymbolicMath&) = delete;
    SymbolicMath(SymbolicMath&&) noexcept;
    SymbolicMath& operator=(SymbolicMath&&) noexcept;

    /**
     * @brief Check if SymPy is available (Python + sympy installed).
     */
    bool isAvailable() const;

    /**
     * @brief Compute Einstein field equations symbolically.
     * @param metric_params Map of metric parameters (e.g., {"M", 1.0} for Schwarzschild)
     * @return SymbolicResult containing the field equation
     */
    SymbolicResult computeEinsteinEquations(
        const std::map<std::string, double>& metric_params
    );

    /**
     * @brief Compute Christoffel symbols symbolically.
     * @param metric_name Name of the metric (e.g., "Schwarzschild", "Kerr")
     * @return SymbolicResult containing Γ^ρ_μν expressions
     */
    SymbolicResult computeChristoffelSymbols(const std::string& metric_name);

    /**
     * @brief Compute Riemann curvature tensor symbolically.
     * @param metric_name Name of the metric
     * @return SymbolicResult containing R^ρ_σμν expressions
     */
    SymbolicResult computeRiemannTensor(const std::string& metric_name);

    /**
     * @brief Compute Ricci scalar symbolically.
     * @param metric_name Name of the metric
     * @return SymbolicResult containing R = g^μν R_μν
     */
    SymbolicResult computeRicciScalar(const std::string& metric_name);

    /**
     * @brief Compute Kretschmann scalar symbolically.
     * @param metric_name Name of the metric
     * @return SymbolicResult containing K = R^μνρσ R_μνρσ
     */
    SymbolicResult computeKretschmannScalar(const std::string& metric_name);

    /**
     * @brief Simplify a symbolic expression.
     * @param expression The expression to simplify
     * @return Simplified expression
     */
    SymbolicResult simplifyExpression(const std::string& expression);

    /**
     * @brief Convert expression to LaTeX format.
     * @param expression The expression to convert
     * @return LaTeX representation
     */
    SymbolicResult toLaTeX(const std::string& expression);

    /**
     * @brief Validate a field equation against known GR solutions.
     * @param field_equation The field equation to validate
     * @return true if valid GR equation
     */
    bool validateFieldEquation(const std::string& field_equation);

    /**
     * @brief Generate a symbolic field equation from trajectory data.
     * Uses numerical data to fit symbolic expressions.
     * @param trajectory_data Vector of (coordinate, value) pairs
     * @return SymbolicResult containing the fitted equation
     */
    SymbolicResult generateFieldEquationFromData(
        const std::vector<std::pair<std::string, double>>& trajectory_data
    );

    /**
     * @brief Execute a Python script and return the result.
     * Made public for use by SymbolicRegressionEngine.
     */
    std::string executePythonScript(const std::string& script) const;

private:
    bool available_;
    std::string python_path_;

    /**
     * @brief Build Python script for Einstein equations.
     */
    std::string buildEinsteinScript(
        const std::map<std::string, double>& params
    ) const;

    /**
     * @brief Build Python script for Christoffel symbols.
     */
    std::string buildChristoffelScript(const std::string& metric_name) const;

    /**
     * @brief Build Python script for curvature tensors.
     */
    std::string buildCurvatureScript(const std::string& metric_name) const;
};

/**
 * @brief Symbolic regression engine for discovering field equations.
 * 
 * Uses genetic programming with SymPy to discover mathematical expressions
 * that fit observational or simulation data.
 */
class SymbolicRegressionEngine {
public:
    /**
     * @brief Configuration for symbolic regression.
     */
    struct Config {
        int population_size;
        int generations;
        int max_expression_depth;
        double fitness_threshold;
        std::vector<std::string> operators;
        std::vector<std::string> variables;
        
        Config()
            : population_size(1000)
            , generations(100)
            , max_expression_depth(10)
            , fitness_threshold(1e-6)
            , operators({"+", "-", "*", "/", "sqrt", "log"})
        {}
    };

    /**
     * @brief Result of symbolic regression.
     */
    struct RegressionResult {
        std::string equation;
        double fitness_score = 0.0;
        int complexity = 0;
        bool is_valid = false;
    };

    explicit SymbolicRegressionEngine(const Config& config = Config{});

    /**
     * @brief Run symbolic regression on data.
     * @param data Input data points (x, y)
     * @return Best fitting equation
     */
    RegressionResult runRegression(
        const std::vector<std::pair<std::vector<double>, double>>& data
    );

    /**
     * @brief Set the configuration.
     */
    void setConfig(const Config& config);

    /**
     * @brief Get the current configuration.
     */
    const Config& getConfig() const { return config_; }

private:
    Config config_;
    SymbolicMath sym_math_;

    std::string buildRegressionScript(
        const std::vector<std::pair<std::vector<double>, double>>& data
    ) const;
};

} // namespace quantumverse