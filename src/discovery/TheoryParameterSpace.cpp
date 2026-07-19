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

        case TheoryType::TE_VES:
            parameters_ = {
                TheoryParameter("K",    0.1, 0.5, 0.3,
                                "TeVeS vector-tensor coupling", "dimensionless"),
                TheoryParameter("mu",   1e-60, 1e-50, 1e-55,
                                "Vector field mass scale", "m^-1"),
                TheoryParameter("sigma", 0.1, 10.0, 1.0,
                                "TeVeS scalar coupling", "dimensionless")
            };
            param_names_ = {"K", "mu", "sigma"};
            break;

        case TheoryType::EINSTEIN_AETHER:
            parameters_ = {
                TheoryParameter("c1", -2.0, 2.0, 0.0,
                                "Einstein-Aether coupling c1", "dimensionless"),
                TheoryParameter("c2", -2.0, 2.0, 0.0,
                                "Einstein-Aether coupling c2", "dimensionless"),
                TheoryParameter("c3", -2.0, 2.0, 0.0,
                                "Einstein-Aether coupling c3", "dimensionless")
            };
            param_names_ = {"c1", "c2", "c3"};
            break;

        case TheoryType::HORNDESKI:
            parameters_ = {
                TheoryParameter("c_G",    -1.0, 1.0, 0.0,
                                "Horndeski PPN deviation", "dimensionless"),
                TheoryParameter("alpha_K", 0.0, 2.0, 0.0,
                                "Kinetic braiding parameter", "dimensionless"),
                TheoryParameter("alpha_B", 0.0, 2.0, 0.0,
                                "Braiding parameter", "dimensionless")
            };
            param_names_ = {"c_G", "alpha_K", "alpha_B"};
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
        case TheoryType::TE_VES:
            return "TeVeS";
        case TheoryType::EINSTEIN_AETHER:
            return "EinsteinAether";
        case TheoryType::HORNDESKI:
            return "Horndeski";
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
