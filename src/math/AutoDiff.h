#ifndef QUANTUMVERSE_AUTODIFF_H
#define QUANTUMVERSE_AUTODIFF_H

#include <cmath>
#include <vector>
#include <array>
#include <functional>
#include <type_traits>
#include <cassert>
#include <memory>
#include <cstdint>
#include <stdexcept>

namespace quantumverse {
namespace math {

template<size_t N = 1>
class ADVariable;

// Forward declarations of free math function templates
// (required for friend declarations inside ADVariable)
template<size_t N> ADVariable<N> sin(const ADVariable<N>& x);
template<size_t N> ADVariable<N> cos(const ADVariable<N>& x);
template<size_t N> ADVariable<N> exp(const ADVariable<N>& x);
template<size_t N> ADVariable<N> log(const ADVariable<N>& x);
template<size_t N> ADVariable<N> sqrt(const ADVariable<N>& x);
template<size_t N> ADVariable<N> pow(const ADVariable<N>& x, double p);
template<size_t N> ADVariable<N> tan(const ADVariable<N>& x);
template<size_t N> ADVariable<N> atan(const ADVariable<N>& x);
template<size_t N> ADVariable<N> sinh(const ADVariable<N>& x);
template<size_t N> ADVariable<N> cosh(const ADVariable<N>& x);
template<size_t N> ADVariable<N> tanh(const ADVariable<N>& x);
template<size_t N> ADVariable<N> asin(const ADVariable<N>& x);
template<size_t N> ADVariable<N> acos(const ADVariable<N>& x);
template<size_t N> ADVariable<N> asinh(const ADVariable<N>& x);
template<size_t N> ADVariable<N> acosh(const ADVariable<N>& x);

/**
 * @file AutoDiff.h
 * @brief QuantumVerse Simulator - Forward-Mode Automatic Differentiation
 *
 * Implements dual numbers for computing exact derivatives through
 * arithmetic expressions and transcendental functions.
 *
 * ADVariable<N> carries a value and an N-dimensional gradient vector.
 * All operations propagate derivatives via the chain rule.
 */
template<size_t N>
class ADVariable {
private:
    double value_;                    // f(x)
    std::array<double, N> derivs_;    // df/dx_i

public:
    // Constructors
    constexpr ADVariable() : value_(0.0) {
        derivs_.fill(0.0);
    }

    constexpr explicit ADVariable(double v) : value_(v) {
        derivs_.fill(0.0);
    }

    constexpr ADVariable(double v, const std::array<double, N>& d) : value_(v), derivs_(d) {}

    constexpr ADVariable(double v, double d1) : value_(v) {
        static_assert(N == 1, "Single derivative requires N=1");
        derivs_[0] = d1;
    }

    // Accessors
    constexpr double getValue() const { return value_; }
    constexpr const std::array<double, N>& getDerivs() const { return derivs_; }
    std::array<double, N>& getDerivs() { return derivs_; }
    constexpr double operator[](size_t i) const { return derivs_[i]; }

    // Basic arithmetic with scalar
    ADVariable operator+(double s) const {
        ADVariable result(value_ + s);
        result.derivs_ = derivs_;
        return result;
    }

    ADVariable operator-(double s) const {
        ADVariable result(value_ - s);
        result.derivs_ = derivs_;
        return result;
    }

    ADVariable operator*(double s) const {
        ADVariable result(value_ * s);
        for (size_t i = 0; i < N; ++i) {
            result.derivs_[i] = derivs_[i] * s;
        }
        return result;
    }

    ADVariable operator/(double s) const {
        ADVariable result(value_ / s);
        for (size_t i = 0; i < N; ++i) {
            result.derivs_[i] = derivs_[i] / s;
        }
        return result;
    }

    // ADVariable + ADVariable
    ADVariable operator+(const ADVariable<N>& other) const {
        ADVariable<N> result(value_ + other.value_);
        for (size_t i = 0; i < N; ++i) {
            result.derivs_[i] = derivs_[i] + other.derivs_[i];
        }
        return result;
    }

    ADVariable operator-(const ADVariable<N>& other) const {
        ADVariable<N> result(value_ - other.value_);
        for (size_t i = 0; i < N; ++i) {
            result.derivs_[i] = derivs_[i] - other.derivs_[i];
        }
        return result;
    }

    ADVariable operator*(const ADVariable<N>& other) const {
        // Product rule: (fg)' = f'g + fg'
        ADVariable<N> result(value_ * other.value_);
        for (size_t i = 0; i < N; ++i) {
            result.derivs_[i] = derivs_[i] * other.value_ + value_ * other.derivs_[i];
        }
        return result;
    }

    ADVariable operator/(const ADVariable<N>& other) const {
        // Quotient rule: (f/g)' = (f'g - fg')/g^2
        ADVariable<N> result(value_ / other.value_);
        for (size_t i = 0; i < N; ++i) {
            result.derivs_[i] = (derivs_[i] * other.value_ - value_ * other.derivs_[i])
                                 / (other.value_ * other.value_);
        }
        return result;
    }

    // Compound assignment with scalar
     ADVariable& operator+=(double s) { value_ += s; return *this; }
     ADVariable& operator-=(double s) { value_ -= s; return *this; }
     ADVariable& operator*=(double s) {
         for (size_t i = 0; i < N; ++i) derivs_[i] *= s;
         value_ *= s;
         return *this;
     }
     ADVariable& operator/=(double s) {
         for (size_t i = 0; i < N; ++i) derivs_[i] /= s;
         value_ /= s;
         return *this;
     }

     // Reverse arithmetic with scalar (enables double * ADVariable)
     friend ADVariable operator+(double s, const ADVariable<N>& var) {
         return var + s;
     }
     friend ADVariable operator-(double s, const ADVariable<N>& var) {
         ADVariable result(s - var.value_);
         for (size_t i = 0; i < N; ++i) result.derivs_[i] = -var.derivs_[i];
         return result;
     }
     friend ADVariable operator*(double s, const ADVariable<N>& var) {
         return var * s;
     }
     friend ADVariable operator/(double s, const ADVariable<N>& var) {
         ADVariable result(s / var.value_);
         for (size_t i = 0; i < N; ++i) result.derivs_[i] = -s * var.derivs_[i] / (var.value_ * var.value_);
         return result;
     }

    // Unary negation
    ADVariable operator-() const {
        ADVariable result(-value_);
        for (size_t i = 0; i < N; ++i) result.derivs_[i] = -derivs_[i];
        return result;
    }

    // Comparison (on value only)
    bool operator==(const ADVariable& other) const { return value_ == other.value_; }
    bool operator!=(const ADVariable& other) const { return value_ != other.value_; }

    // Grant free math functions access to private members
    friend ADVariable<N> sin<>(const ADVariable<N>& x);
    friend ADVariable<N> cos<>(const ADVariable<N>& x);
    friend ADVariable<N> exp<>(const ADVariable<N>& x);
    friend ADVariable<N> log<>(const ADVariable<N>& x);
    friend ADVariable<N> sqrt<>(const ADVariable<N>& x);
    friend ADVariable<N> pow<>(const ADVariable<N>& x, double p);
    friend ADVariable<N> tan<>(const ADVariable<N>& x);
    friend ADVariable<N> atan<>(const ADVariable<N>& x);
    friend ADVariable<N> sinh<>(const ADVariable<N>& x);
    friend ADVariable<N> cosh<>(const ADVariable<N>& x);
    friend ADVariable<N> tanh<>(const ADVariable<N>& x);
    friend ADVariable<N> asin<>(const ADVariable<N>& x);
    friend ADVariable<N> acos<>(const ADVariable<N>& x);
    friend ADVariable<N> asinh<>(const ADVariable<N>& x);
    friend ADVariable<N> acosh<>(const ADVariable<N>& x);

    // Utility: create independent variable
    static ADVariable variable(double v, size_t index) {
        ADVariable var(v);
        var.derivs_.fill(0.0);
        if (index < N) var.derivs_[index] = 1.0;
        return var;
    }

    // Utility: create constant
    static ADVariable constant(double v) { return ADVariable(v); }
};

// ========== Transcendental Functions ==========

template<size_t N>
ADVariable<N> sin(const ADVariable<N>& x) {
    ADVariable<N> result(std::sin(x.value_));
    for (size_t i = 0; i < N; ++i) {
        result.derivs_[i] = std::cos(x.value_) * x.derivs_[i];
    }
    return result;
}

template<size_t N>
ADVariable<N> cos(const ADVariable<N>& x) {
    ADVariable<N> result(std::cos(x.value_));
    for (size_t i = 0; i < N; ++i) {
        result.derivs_[i] = -std::sin(x.value_) * x.derivs_[i];
    }
    return result;
}

template<size_t N>
ADVariable<N> exp(const ADVariable<N>& x) {
    ADVariable<N> result(std::exp(x.value_));
    for (size_t i = 0; i < N; ++i) {
        result.derivs_[i] = result.value_ * x.derivs_[i];
    }
    return result;
}

template<size_t N>
ADVariable<N> log(const ADVariable<N>& x) {
    ADVariable<N> result(std::log(x.value_));
    for (size_t i = 0; i < N; ++i) {
        result.derivs_[i] = x.derivs_[i] / x.value_;
    }
    return result;
}

template<size_t N>
ADVariable<N> sqrt(const ADVariable<N>& x) {
    ADVariable<N> result(std::sqrt(x.value_));
    for (size_t i = 0; i < N; ++i) {
        result.derivs_[i] = 0.5 / result.value_ * x.derivs_[i];
    }
    return result;
}

template<size_t N>
ADVariable<N> pow(const ADVariable<N>& x, double p) {
    ADVariable<N> result(std::pow(x.value_, p));
    for (size_t i = 0; i < N; ++i) {
        result.derivs_[i] = p * std::pow(x.value_, p - 1) * x.derivs_[i];
    }
    return result;
}

template<size_t N>
ADVariable<N> pow(double base, const ADVariable<N>& x) {
    return exp(x * ADVariable<N>::constant(std::log(base)));
}

template<size_t N>
ADVariable<N> pow(const ADVariable<N>& x, const ADVariable<N>& y) {
    return exp(y * log(x));
}

// ========== Additional Transcendental Functions ==========

template<size_t N>
ADVariable<N> tan(const ADVariable<N>& x) {
    double tx = std::tan(x.value_);
    ADVariable<N> result(tx);
    for (size_t i = 0; i < N; ++i) {
        result.derivs_[i] = (1.0 + tx * tx) * x.derivs_[i];
    }
    return result;
}

template<size_t N>
ADVariable<N> atan(const ADVariable<N>& x) {
    ADVariable<N> result(std::atan(x.value_));
    for (size_t i = 0; i < N; ++i) {
        result.derivs_[i] = x.derivs_[i] / (1.0 + x.value_ * x.value_);
    }
    return result;
}

template<size_t N>
ADVariable<N> sinh(const ADVariable<N>& x) {
    ADVariable<N> result(std::sinh(x.value_));
    for (size_t i = 0; i < N; ++i) {
        result.derivs_[i] = std::cosh(x.value_) * x.derivs_[i];
    }
    return result;
}

template<size_t N>
ADVariable<N> cosh(const ADVariable<N>& x) {
    ADVariable<N> result(std::cosh(x.value_));
    for (size_t i = 0; i < N; ++i) {
        result.derivs_[i] = std::sinh(x.value_) * x.derivs_[i];
    }
    return result;
}

template<size_t N>
ADVariable<N> tanh(const ADVariable<N>& x) {
    double tx = std::tanh(x.value_);
    ADVariable<N> result(tx);
    for (size_t i = 0; i < N; ++i) {
        result.derivs_[i] = (1.0 - tx * tx) * x.derivs_[i];
    }
    return result;
}

template<size_t N>
ADVariable<N> asin(const ADVariable<N>& x) {
    ADVariable<N> result(std::asin(x.value_));
    for (size_t i = 0; i < N; ++i) {
        result.derivs_[i] = x.derivs_[i] / std::sqrt(1.0 - x.value_ * x.value_);
    }
    return result;
}

template<size_t N>
ADVariable<N> acos(const ADVariable<N>& x) {
    ADVariable<N> result(std::acos(x.value_));
    for (size_t i = 0; i < N; ++i) {
        result.derivs_[i] = -x.derivs_[i] / std::sqrt(1.0 - x.value_ * x.value_);
    }
    return result;
}

template<size_t N>
ADVariable<N> asinh(const ADVariable<N>& x) {
    ADVariable<N> result(std::asinh(x.value_));
    for (size_t i = 0; i < N; ++i) {
        result.derivs_[i] = x.derivs_[i] / std::sqrt(1.0 + x.value_ * x.value_);
    }
    return result;
}

template<size_t N>
ADVariable<N> acosh(const ADVariable<N>& x) {
    ADVariable<N> result(std::acosh(x.value_));
    for (size_t i = 0; i < N; ++i) {
        result.derivs_[i] = x.derivs_[i] / std::sqrt(x.value_ * x.value_ - 1.0);
    }
    return result;
}

// ========== Gradient / Jacobian Utilities ==========

template<size_t N>
std::array<double, N> gradient(
    const std::function<ADVariable<N>(const std::vector<ADVariable<N>>&)>& f,
    const std::vector<double>& x
) {
    assert(x.size() == N);
    std::vector<ADVariable<N>> vars;
    for (size_t i = 0; i < N; ++i) {
        vars.push_back(ADVariable<N>::variable(x[i], i));
    }
    ADVariable<N> result = f(vars);
    std::array<double, N> grad{};
    for (size_t i = 0; i < N; ++i) {
        grad[i] = result[i];
    }
    return grad;
}

template<size_t N, size_t M>
std::array<std::array<double, M>, N> jacobian(
    const std::function<std::vector<ADVariable<N>>(const std::vector<ADVariable<M>>&)>& F,
    const std::vector<double>& x
) {
    assert(x.size() == M);
    std::vector<ADVariable<M>> vars;
    for (size_t i = 0; i < M; ++i) {
        vars.push_back(ADVariable<M>::variable(x[i], i));
    }
    std::vector<ADVariable<N>> result = F(vars);
    std::array<std::array<double, M>, N> J{};
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < M; ++j) {
            J[i][j] = result[i][j];
        }
    }
    return J;
}

// ========== Scalar Convenience Wrappers ==========

inline double differentiate(
    const std::function<double(double)>& f,
    double x,
    double h = 1e-5
) {
    return (f(x + h) - f(x - h)) / (2 * h);
}

template<size_t N>
std::array<double, N> gradientN(
    const std::function<double(const std::vector<double>&)>& f,
    const std::vector<double>& x
) {
    std::array<double, N> grad{};
    const double h = 1e-8;
    
    for (size_t i = 0; i < N; ++i) {
        std::vector<double> x_plus = x;
        std::vector<double> x_minus = x;
        x_plus[i] += h;
        x_minus[i] -= h;
        grad[i] = (f(x_plus) - f(x_minus)) / (2.0 * h);
    }
    return grad;
}

// ============================================================================
// Reverse-Mode Automatic Differentiation (Adjoint Method)
// ============================================================================

class ADVar {
public:
    double value;
    double grad;
    std::function<void()> backward;

    ADVar(double v) : value(v), grad(0.0) {}

    ADVar(double v, std::function<void()> bw) : value(v), grad(0.0), backward(std::move(bw)) {}

    ADVar(const ADVar&) = delete;
    ADVar& operator=(const ADVar&) = delete;
    ADVar(ADVar&& other) noexcept
        : value(other.value), grad(other.grad), backward(std::move(other.backward)) {}
};

class ADTape {
    static std::vector<std::unique_ptr<ADVar>> variables;

public:
    static void push(ADVar* var) { variables.push_back(std::unique_ptr<ADVar>(var)); }

    static void clear() { variables.clear(); }

    static ADVar* record(double value, std::function<void()> backward_fn) {
        auto var = std::make_unique<ADVar>(value, std::move(backward_fn));
        ADVar* raw = var.get();
        variables.push_back(std::move(var));
        return raw;
    }

    static std::vector<std::pair<ADVar*, double>> compute_gradients(ADVar& output) {
        output.grad = 1.0;
        for (auto it = variables.rbegin(); it != variables.rend(); ++it) {
            if ((*it)->backward) (*it)->backward();
        }
        std::vector<std::pair<ADVar*, double>> grads;
        grads.reserve(variables.size());
        for (auto& v : variables) grads.emplace_back(v.get(), v->grad);
        return grads;
    }

    static double get_gradient(ADVar* var, const std::vector<std::pair<ADVar*, double>>& grads) {
        for (const auto& [v, g] : grads) {
            if (v == var) return g;
        }
        return 0.0;
    }

    static const std::vector<std::unique_ptr<ADVar>>& getVariables() { return variables; }
};

inline std::vector<std::unique_ptr<ADVar>> ADTape::variables;

inline ADVar* var(double v) {
    return ADTape::record(v, nullptr);
}

inline ADVar* add(ADVar* a, ADVar* b) {
    ADVar* result = new ADVar(a->value + b->value);
    result->backward = [a, b, result]() {
        double dc = result->grad;
        a->grad += dc;
        b->grad += dc;
    };
    ADTape::push(result);
    return result;
}

inline ADVar* sub(ADVar* a, ADVar* b) {
    ADVar* result = new ADVar(a->value - b->value);
    result->backward = [a, b, result]() {
        double dc = result->grad;
        a->grad += dc;
        b->grad += -dc;
    };
    ADTape::push(result);
    return result;
}

inline ADVar* mul(ADVar* a, ADVar* b) {
    ADVar* result = new ADVar(a->value * b->value);
    result->backward = [a, b, result]() {
        double dc = result->grad;
        a->grad += dc * b->value;
        b->grad += dc * a->value;
    };
    ADTape::push(result);
    return result;
}

inline ADVar* div(ADVar* a, ADVar* b) {
    ADVar* result = new ADVar(a->value / b->value);
    result->backward = [a, b, result]() {
        double dc = result->grad;
        a->grad += dc / b->value;
        b->grad += -dc * a->value / (b->value * b->value);
    };
    ADTape::push(result);
    return result;
}

inline ADVar* add(ADVar* a, double s) {
    ADVar* result = new ADVar(a->value + s);
    result->backward = [a, result]() {
        a->grad += result->grad;
    };
    ADTape::push(result);
    return result;
}

inline ADVar* add(double s, ADVar* a) {
    return add(a, s);
}

inline ADVar* sub(ADVar* a, double s) {
    ADVar* result = new ADVar(a->value - s);
    result->backward = [a, result]() {
        a->grad += result->grad;
    };
    ADTape::push(result);
    return result;
}

inline ADVar* sub(double s, ADVar* a) {
    ADVar* result = new ADVar(s - a->value);
    result->backward = [a, result]() {
        a->grad += -result->grad;
    };
    ADTape::push(result);
    return result;
}

inline ADVar* mul(ADVar* a, double s) {
    ADVar* result = new ADVar(a->value * s);
    result->backward = [a, s, result]() {
        a->grad += result->grad * s;
    };
    ADTape::push(result);
    return result;
}

inline ADVar* mul(double s, ADVar* a) {
    return mul(a, s);
}

inline ADVar* div(ADVar* a, double s) {
    ADVar* result = new ADVar(a->value / s);
    result->backward = [a, s, result]() {
        a->grad += result->grad / s;
    };
    ADTape::push(result);
    return result;
}

inline ADVar* div(double s, ADVar* a) {
    ADVar* result = new ADVar(s / a->value);
    result->backward = [a, s, result]() {
        a->grad += -result->grad * s / (a->value * a->value);
    };
    ADTape::push(result);
    return result;
}

inline ADVar* neg(ADVar* a) {
    ADVar* result = new ADVar(-a->value);
    result->backward = [a, result]() {
        a->grad += -result->grad;
    };
    ADTape::push(result);
    return result;
}

inline ADVar* sin(ADVar* x) {
    ADVar* result = new ADVar(std::sin(x->value));
    result->backward = [x, result]() {
        x->grad += result->grad * std::cos(x->value);
    };
    ADTape::push(result);
    return result;
}

inline ADVar* cos(ADVar* x) {
    ADVar* result = new ADVar(std::cos(x->value));
    result->backward = [x, result]() {
        x->grad += result->grad * -std::sin(x->value);
    };
    ADTape::push(result);
    return result;
}

inline ADVar* exp(ADVar* x) {
    ADVar* result = new ADVar(std::exp(x->value));
    result->backward = [x, result]() {
        x->grad += result->grad * result->value;
    };
    ADTape::push(result);
    return result;
}

inline ADVar* log(ADVar* x) {
    ADVar* result = new ADVar(std::log(x->value));
    result->backward = [x, result]() {
        x->grad += result->grad / x->value;
    };
    ADTape::push(result);
    return result;
}

inline ADVar* sqrt(ADVar* x) {
    ADVar* result = new ADVar(std::sqrt(x->value));
    result->backward = [x, result]() {
        x->grad += result->grad * 0.5 / result->value;
    };
    ADTape::push(result);
    return result;
}

inline ADVar* pow(ADVar* x, double p) {
    ADVar* result = new ADVar(std::pow(x->value, p));
    result->backward = [x, p, result]() {
        x->grad += result->grad * p * std::pow(x->value, p - 1);
    };
    ADTape::push(result);
    return result;
}

inline ADVar* pow(double base, ADVar* x) {
    ADVar* result = new ADVar(std::pow(base, x->value));
    result->backward = [x, base, result]() {
        x->grad += result->grad * std::pow(base, x->value) * std::log(base);
    };
    ADTape::push(result);
    return result;
}

inline ADVar* tan(ADVar* x) {
    double tx = std::tan(x->value);
    ADVar* result = new ADVar(tx);
    result->backward = [x, result, tx]() {
        x->grad += result->grad * (1.0 + tx * tx);
    };
    ADTape::push(result);
    return result;
}

inline ADVar* atan(ADVar* x) {
    ADVar* result = new ADVar(std::atan(x->value));
    result->backward = [x, result]() {
        x->grad += result->grad / (1.0 + x->value * x->value);
    };
    ADTape::push(result);
    return result;
}

inline ADVar* sinh(ADVar* x) {
    ADVar* result = new ADVar(std::sinh(x->value));
    result->backward = [x, result]() {
        x->grad += result->grad * std::cosh(x->value);
    };
    ADTape::push(result);
    return result;
}

inline ADVar* cosh(ADVar* x) {
    ADVar* result = new ADVar(std::cosh(x->value));
    result->backward = [x, result]() {
        x->grad += result->grad * std::sinh(x->value);
    };
    ADTape::push(result);
    return result;
}

inline ADVar* tanh(ADVar* x) {
    double tx = std::tanh(x->value);
    ADVar* result = new ADVar(tx);
    result->backward = [x, result, tx]() {
        x->grad += result->grad * (1.0 - tx * tx);
    };
    ADTape::push(result);
    return result;
}

inline ADVar* asin(ADVar* x) {
    ADVar* result = new ADVar(std::asin(x->value));
    result->backward = [x, result]() {
        x->grad += result->grad / std::sqrt(1.0 - x->value * x->value);
    };
    ADTape::push(result);
    return result;
}

inline ADVar* acos(ADVar* x) {
    ADVar* result = new ADVar(std::acos(x->value));
    result->backward = [x, result]() {
        x->grad += result->grad / -std::sqrt(1.0 - x->value * x->value);
    };
    ADTape::push(result);
    return result;
}

inline ADVar* asinh(ADVar* x) {
    ADVar* result = new ADVar(std::asinh(x->value));
    result->backward = [x, result]() {
        x->grad += result->grad / std::sqrt(1.0 + x->value * x->value);
    };
    ADTape::push(result);
    return result;
}

inline ADVar* acosh(ADVar* x) {
    ADVar* result = new ADVar(std::acosh(x->value));
    result->backward = [x, result]() {
        x->grad += result->grad / std::sqrt(x->value * x->value - 1.0);
    };
    ADTape::push(result);
    return result;
}

// ============================================================================
// Reverse-Mode Automatic Differentiation (Tape-Based, N-Parameter)
// ============================================================================

template<size_t N>
class ReverseTape {
public:
    struct Node {
        double value = 0.0;
        uint32_t p0 = UINT32_MAX, p1 = UINT32_MAX;
        double w0 = 0.0, w1 = 0.0;
    };

    std::vector<Node> nodes;
    std::array<uint32_t, N> paramNodes;
    std::array<bool, N> paramSet;
    std::vector<double> adjoints;

    ReverseTape() {
        paramNodes.fill(UINT32_MAX);
        paramSet.fill(false);
    }

    void reserve(size_t n) {
        nodes.reserve(n);
        adjoints.resize(n);
    }

    uint32_t push(double v) {
        nodes.push_back({v, UINT32_MAX, UINT32_MAX, 0.0, 0.0});
        return static_cast<uint32_t>(nodes.size() - 1);
    }

    uint32_t makeParam(double v, size_t idx) {
        if (idx >= N) throw std::out_of_range("ReverseTape::makeParam: index out of range");
        auto id = push(v);
        paramNodes[idx] = id;
        paramSet[idx] = true;
        return id;
    }

    /// Backprop from `root`, return d(root)/d(theta_i) for all tagged params.
    /// Allocation order == topological order, so reverse iteration is a valid
    /// reverse-topological pass.
    std::array<double, N> backward(uint32_t root) {
        if (root == UINT32_MAX || nodes.empty()) {
            // No tape recorded — return zero gradient
            return {};
        }
        adjoints.assign(nodes.size(), 0.0);
        adjoints[root] = 1.0;

        for (int i = static_cast<int>(nodes.size()) - 1; i >= 0; --i) {
            const auto& n = nodes[i];
            double a = adjoints[i];
            if (a == 0.0) continue;
            if (n.p0 != UINT32_MAX) adjoints[n.p0] += a * n.w0;
            if (n.p1 != UINT32_MAX) adjoints[n.p1] += a * n.w1;
        }

        std::array<double, N> g{};
        for (size_t i = 0; i < N; ++i)
            if (paramSet[i]) g[i] = adjoints[paramNodes[i]];
        return g;
    }

    void clear() {
        nodes.clear();
        paramNodes.fill(UINT32_MAX);
        paramSet.fill(false);
        adjoints.clear();
    }
};

template<size_t N>
struct ReverseAD {
    ReverseTape<N>* tape = nullptr;
    uint32_t node = UINT32_MAX;
    double value = 0.0;

    ReverseAD() = default;

    static ReverseAD param(ReverseTape<N>& t, double v, size_t idx) {
        return { &t, t.makeParam(v, idx), v };
    }

    static ReverseAD constant(double v) {
        return { nullptr, UINT32_MAX, v };
    }

    // ---------- binary ops ----------
    friend ReverseAD operator+(const ReverseAD& a, const ReverseAD& b) {
        ReverseAD r;
        r.value = a.value + b.value;
        if (a.tape && b.tape && a.tape == b.tape) {
            r.tape = a.tape;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = b.node;
            r.tape->nodes[id].w0 = 1.0;
            r.tape->nodes[id].w1 = 1.0;
        } else if (a.tape) {
            r.tape = a.tape;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = 1.0;
            r.tape->nodes[id].w1 = 0.0;
        } else if (b.tape) {
            r.tape = b.tape;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = UINT32_MAX;
            r.tape->nodes[id].p1 = b.node;
            r.tape->nodes[id].w0 = 0.0;
            r.tape->nodes[id].w1 = 1.0;
        }
        return r;
    }

    friend ReverseAD operator-(const ReverseAD& a, const ReverseAD& b) {
        ReverseAD r;
        r.value = a.value - b.value;
        if (a.tape && b.tape && a.tape == b.tape) {
            r.tape = a.tape;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = b.node;
            r.tape->nodes[id].w0 = 1.0;
            r.tape->nodes[id].w1 = -1.0;
        } else if (a.tape) {
            r.tape = a.tape;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = 1.0;
            r.tape->nodes[id].w1 = 0.0;
        } else if (b.tape) {
            r.tape = b.tape;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = UINT32_MAX;
            r.tape->nodes[id].p1 = b.node;
            r.tape->nodes[id].w0 = 0.0;
            r.tape->nodes[id].w1 = -1.0;
        }
        return r;
    }

    friend ReverseAD operator*(const ReverseAD& a, const ReverseAD& b) {
        ReverseAD r;
        r.value = a.value * b.value;
        if (a.tape && b.tape && a.tape == b.tape) {
            r.tape = a.tape;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = b.node;
            r.tape->nodes[id].w0 = b.value;
            r.tape->nodes[id].w1 = a.value;
        } else if (a.tape) {
            r.tape = a.tape;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = b.value;
            r.tape->nodes[id].w1 = 0.0;
        } else if (b.tape) {
            r.tape = b.tape;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = UINT32_MAX;
            r.tape->nodes[id].p1 = b.node;
            r.tape->nodes[id].w0 = 0.0;
            r.tape->nodes[id].w1 = a.value;
        }
        return r;
    }

    friend ReverseAD operator/(const ReverseAD& a, const ReverseAD& b) {
        ReverseAD r;
        r.value = a.value / b.value;
        if (a.tape && b.tape && a.tape == b.tape) {
            r.tape = a.tape;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = b.node;
            r.tape->nodes[id].w0 = 1.0 / b.value;
            r.tape->nodes[id].w1 = -a.value / (b.value * b.value);
        } else if (a.tape) {
            r.tape = a.tape;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = 1.0 / b.value;
            r.tape->nodes[id].w1 = 0.0;
        } else if (b.tape) {
            r.tape = b.tape;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = UINT32_MAX;
            r.tape->nodes[id].p1 = b.node;
            r.tape->nodes[id].w0 = 0.0;
            r.tape->nodes[id].w1 = -a.value / (b.value * b.value);
        }
        return r;
    }

    // ---------- scalar ops ----------
    friend ReverseAD operator+(const ReverseAD& a, double s) {
        ReverseAD r;
        if (a.tape) {
            r.tape = a.tape;
            r.value = a.value + s;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = 1.0;
            r.tape->nodes[id].w1 = 0.0;
        } else {
            r.value = a.value + s;
        }
        return r;
    }

    friend ReverseAD operator+(double s, const ReverseAD& a) { return a + s; }

    friend ReverseAD operator-(const ReverseAD& a, double s) {
        ReverseAD r;
        if (a.tape) {
            r.tape = a.tape;
            r.value = a.value - s;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = 1.0;
            r.tape->nodes[id].w1 = 0.0;
        } else {
            r.value = a.value - s;
        }
        return r;
    }

    friend ReverseAD operator-(double s, const ReverseAD& a) {
        ReverseAD r;
        if (a.tape) {
            r.tape = a.tape;
            r.value = s - a.value;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = -1.0;
            r.tape->nodes[id].w1 = 0.0;
        } else {
            r.value = s - a.value;
        }
        return r;
    }

    friend ReverseAD operator*(const ReverseAD& a, double s) {
        ReverseAD r;
        if (a.tape) {
            r.tape = a.tape;
            r.value = a.value * s;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = s;
            r.tape->nodes[id].w1 = 0.0;
        } else {
            r.value = a.value * s;
        }
        return r;
    }

    friend ReverseAD operator*(double s, const ReverseAD& a) { return a * s; }

    friend ReverseAD operator/(const ReverseAD& a, double s) {
        ReverseAD r;
        if (a.tape) {
            r.tape = a.tape;
            r.value = a.value / s;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = 1.0 / s;
            r.tape->nodes[id].w1 = 0.0;
        } else {
            r.value = a.value / s;
        }
        return r;
    }

    friend ReverseAD operator/(double s, const ReverseAD& a) {
        ReverseAD r;
        if (a.tape) {
            r.tape = a.tape;
            r.value = s / a.value;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = -s / (a.value * a.value);
            r.tape->nodes[id].w1 = 0.0;
        } else {
            r.value = s / a.value;
        }
        return r;
    }

    // ---------- unary ----------
    friend ReverseAD operator-(const ReverseAD& a) {
        ReverseAD r;
        if (a.tape) {
            r.tape = a.tape;
            r.value = -a.value;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = -1.0;
            r.tape->nodes[id].w1 = 0.0;
        } else {
            r.value = -a.value;
        }
        return r;
    }

    // ---------- transcendental ----------
    friend ReverseAD sin(const ReverseAD& a) {
        ReverseAD r;
        if (a.tape) {
            r.tape = a.tape;
            double cv = std::cos(a.value);
            r.value = std::sin(a.value);
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = cv;
            r.tape->nodes[id].w1 = 0.0;
        } else {
            r.value = std::sin(a.value);
        }
        return r;
    }

    friend ReverseAD cos(const ReverseAD& a) {
        ReverseAD r;
        if (a.tape) {
            r.tape = a.tape;
            double sv = std::sin(a.value);
            r.value = std::cos(a.value);
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = -sv;
            r.tape->nodes[id].w1 = 0.0;
        } else {
            r.value = std::cos(a.value);
        }
        return r;
    }

    friend ReverseAD exp(const ReverseAD& a) {
        ReverseAD r;
        if (a.tape) {
            r.tape = a.tape;
            r.value = std::exp(a.value);
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = r.value;
            r.tape->nodes[id].w1 = 0.0;
        } else {
            r.value = std::exp(a.value);
        }
        return r;
    }

    friend ReverseAD log(const ReverseAD& a) {
        ReverseAD r;
        if (a.tape) {
            r.tape = a.tape;
            r.value = std::log(a.value);
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = 1.0 / a.value;
            r.tape->nodes[id].w1 = 0.0;
        } else {
            r.value = std::log(a.value);
        }
        return r;
    }

    friend ReverseAD sqrt(const ReverseAD& a) {
        ReverseAD r;
        if (a.tape) {
            r.tape = a.tape;
            double rv = std::sqrt(a.value);
            r.value = rv;
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = 0.5 / rv;
            r.tape->nodes[id].w1 = 0.0;
        } else {
            r.value = std::sqrt(a.value);
        }
        return r;
    }

    friend ReverseAD pow(const ReverseAD& a, double p) {
        ReverseAD r;
        if (a.tape) {
            r.tape = a.tape;
            r.value = std::pow(a.value, p);
            auto id = r.tape->push(r.value);
            r.node = id;
            r.tape->nodes[id].p0 = a.node;
            r.tape->nodes[id].p1 = UINT32_MAX;
            r.tape->nodes[id].w0 = p * std::pow(a.value, p - 1.0);
            r.tape->nodes[id].w1 = 0.0;
        } else {
            r.value = std::pow(a.value, p);
        }
        return r;
    }
};

} // namespace math
} // namespace quantumverse

#endif // QUANTUMVERSE_AUTODIFF_H
