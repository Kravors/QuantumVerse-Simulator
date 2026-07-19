#include "TheoryParameterSpace.h"

namespace quantumverse {
namespace discovery {

TheoryParameterSpace::TheoryParameterSpace(TheoryType type)
    : type_(type)
{
    switch (type_) {
        case TheoryType::FR_GRAVITY:
            parameters_ = {
                TheoryParameter("alpha", -2.0, 2.0, 1.0,
                                "f(R) coupling strength", "dimensionless"),
                TheoryParameter("n",      -2.0, 2.0, 1.0,
                                "Power-law exponent for f(R)", "dimensionless")
            };
            param_names_ = {"alpha", "n"};
            break;

        case TheoryType::BRANS_DICKE:
            parameters_ = {
                TheoryParameter("omega",  1.0, 100000.0, 40000.0,
                                "Brans-Dicke coupling", "dimensionless"),
                TheoryParameter("phi0",   0.1, 10.0, 1.0,
                                "Background scalar field value", "dimensionless")
            };
            param_names_ = {"omega", "phi0"};
            break;

        case TheoryType::LOOP_QUANTUM_GRAVITY:
            parameters_ = {
                TheoryParameter("gamma",  0.1, 0.5, 0.2375,
                                "Immirzi parameter", "dimensionless"),
                TheoryParameter("lambda", 1e-40, 1e-30, 1.616e-35,
                                "Polymer length scale", "meters")
            };
            param_names_ = {"gamma", "lambda"};
            break;

        case TheoryType::CUSTOM:
        default:
            parameters_ = {
                TheoryParameter("p0", -1.0, 1.0, 0.0,
                                "Generic parameter 0", "dimensionless"),
                TheoryParameter("p1", -1.0, 1.0, 0.0,
                                "Generic parameter 1", "dimensionless")
            };
            param_names_ = {"p0", "p1"};
            break;
    }
}

std::vector<TheoryParameter> TheoryParameterSpace::getParameters() const {
    return parameters_;
}

int TheoryParameterSpace::getParameterDimension() const {
    return static_cast<int>(parameters_.size());
}

std::map<std::string, double> TheoryParameterSpace::parameterVectorToMap(
    const std::vector<double>& vec
) const {
    std::map<std::string, double> result;
    int dim = static_cast<int>(parameters_.size());
    for (int i = 0; i < dim && i < static_cast<int>(vec.size()); ++i) {
        double v = vec[i];
        if (v < parameters_[i].min) v = parameters_[i].min;
        if (v > parameters_[i].max) v = parameters_[i].max;
        result[parameters_[i].name] = v;
    }
    return result;
}

std::vector<double> TheoryParameterSpace::mapToParameterVector(
    const std::map<std::string, double>& map
) const {
    std::vector<double> vec;
    vec.reserve(parameters_.size());
    for (const auto& param : parameters_) {
        auto it = map.find(param.name);
        double v = (it != map.end()) ? it->second : param.default_value;
        vec.push_back(v);
    }
    return vec;
}

std::string TheoryParameterSpace::getTheoryName() const {
    switch (type_) {
        case TheoryType::FR_GRAVITY:
            return "FRLGravity";
        case TheoryType::BRANS_DICKE:
            return "BransDicke";
        case TheoryType::LOOP_QUANTUM_GRAVITY:
            return "LQG";
        case TheoryType::CUSTOM:
        default:
            return "CustomTheory";
    }
}

TheoryParameterSpace::TheoryType TheoryParameterSpace::getTheoryType() const {
    return type_;
}

} // namespace discovery
} // namespace quantumverse
