#ifndef QUANTUMVERSE_AUTODIFF_H
#define QUANTUMVERSE_AUTODIFF_H

#include <cmath>
#include <vector>
#include <array>
#include <functional>
#include <type_traits>
#include <cassert>

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
    std::array<double, N> grad;
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
    std::array<std::array<double, M>, N> J;
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < M; ++j) {
            J[i][j] = result[i][j];
        }
    }
    return J;
}

// ========== Scalar Convenience Wrappers ==========

double differentiate(
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
    // Use finite difference for scalar functions (AD requires expression templates)
    // This is a simple and robust implementation
    std::array<double, N> grad;
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

} // namespace math
} // namespace quantumverse

#endif // QUANTUMVERSE_AUTODIFF_H
