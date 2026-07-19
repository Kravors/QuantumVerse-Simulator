#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <cstdio>
#include <cctype>
#include <map>
#include <vector>

namespace quantumverse {

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static double to_double(const std::string& s, bool& ok) {
    ok = false;
    if (s.empty()) return 0.0;
    char* end = nullptr;
    double v = std::strtod(s.c_str(), &end);
    if (end == s.c_str() || *end != '\0') return 0.0;
    ok = true;
    return v;
}

static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    return out;
}

static std::string json_number(double v) {
    if (std::isnan(v)) return "\"NaN\"";
    if (std::isinf(v)) return v > 0 ? "\"Infinity\"" : "\"-Infinity\"";
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.17g", v);
    return buf;
}

// ---------------------------------------------------------------------------
// Schwarzschild exact invariants in geometric units (G = c = 1)
// ---------------------------------------------------------------------------
struct SchwarzschildInvariants {
    double ricciScalar;
    double kretschmann;
    bool valid;
};

static SchwarzschildInvariants schwarzschild_exact(double M, double r, double theta, double phi) {
    (void)theta;
    (void)phi;
    SchwarzschildInvariants out;
    const double minR = 1e-12;
    r = std::max(r, minR);

    const double K = 48.0 * M * M / (r * r * r * r * r * r);
    const double R = 0.0;

    out.ricciScalar = R;
    out.kretschmann = K;
    out.valid = std::isfinite(K) && std::isfinite(R);
    return out;
}

// ---------------------------------------------------------------------------
// Kerr exact invariants in geometric units (G = c = 1)
// ---------------------------------------------------------------------------
// K = 48 M^2 (r^2 - a^2 cos^2(theta)) * ((r^2 + a^2 cos^2(theta))^2 - 16 r^2 a^2 cos^2(theta))
//     ------------------------------------------------------------------------------------------------
//                                (r^2 + a^2 cos^2(theta))^6
// Ricci scalar = 0 (vacuum)
// ---------------------------------------------------------------------------
struct KerrInvariants {
    double ricciScalar;
    double kretschmann;
    bool valid;
};

static KerrInvariants kerr_exact(double M, double a, double r, double theta) {
    KerrInvariants out;
    const double minR = 1e-12;
    r = std::max(r, minR);

    // Reject super-extremal Kerr (naked singularity)
    if (a > M) {
        out.valid = false;
        return out;
    }

    const double cosTheta = std::cos(theta);
    const double cos2 = cosTheta * cosTheta;
    const double r2 = r * r;
    const double a2 = a * a;
    const double sigma = r2 + a2 * cos2;
    const double sigma2 = sigma * sigma;
    const double sigma6 = sigma2 * sigma2 * sigma2;

    const double num1 = r2 - a2 * cos2;
    const double num2 = sigma2 - 16.0 * r2 * a2 * cos2;
    const double K = 48.0 * M * M * num1 * num2 / sigma6;

    out.ricciScalar = 0.0;
    out.kretschmann = K;
    out.valid = std::isfinite(K) && sigma6 > 0 && num1 != 0;
    return out;
}

// ---------------------------------------------------------------------------
// FLRW flat matter-dominated exact invariants in geometric units (G = c = 1)
// ---------------------------------------------------------------------------
// a(t) = (t/t0)^(2/3)
// H = 2/(3t)
// R = 4/(3t^2)
// K = 16/(27 t^4)
// ---------------------------------------------------------------------------
struct FLRWInvariants {
    double ricciScalar;
    double kretschmann;
    bool valid;
};

static FLRWInvariants flrw_matter_exact(double t, double t0) {
    FLRWInvariants out;
    if (t <= 0.0 || t0 <= 0.0) {
        out.valid = false;
        return out;
    }

    const double t2 = t * t;
    const double t4 = t2 * t2;
    const double R = 4.0 / (3.0 * t2);
    const double K = 16.0 / (27.0 * t4);

    out.ricciScalar = R;
    out.kretschmann = K;
    out.valid = std::isfinite(R) && std::isfinite(K);
    return out;
}

// ---------------------------------------------------------------------------
// CGHS 2D dilaton exact invariants in geometric units (G = c = 1)
// ---------------------------------------------------------------------------
// Conformal gauge: ds^2 = -e^{2rho} dx^+ dx^-
// e^{2rho} = 1 + M e^{2sigma} x^+ x^-
// R = 4 M e^{2sigma} / (1 + M e^{2sigma} x^+ x^-)^3
// In 2D: K = R^2 (no Weyl tensor)
// ---------------------------------------------------------------------------
struct CGHSInvariants {
    double ricciScalar;
    double kretschmann;
    bool valid;
};

static CGHSInvariants cghs_exact(double M, double sigma, double x_plus, double x_minus) {
    CGHSInvariants out;
    const double A = 1.0 + M * std::exp(2.0 * sigma) * x_plus * x_minus;
    if (A <= 0.0) {
        out.valid = false;
        return out;
    }

    const double R = 4.0 * M * std::exp(2.0 * sigma) / (A * A * A);
    const double K = R * R;

    out.ricciScalar = R;
    out.kretschmann = K;
    out.valid = std::isfinite(R) && std::isfinite(K);
    return out;
}

// ---------------------------------------------------------------------------
// JSON input parser
// ---------------------------------------------------------------------------
static bool handle(const std::string& input) {
    std::string metric;
    std::map<std::string,double> params;
    double t=0, x=0, y=0, z=0;

    auto p1 = input.find("\"metric\"");
    if (p1 == std::string::npos) {
        std::cout << "{\"error\":\"missing metric\"}\n";
        return false;
    }
    auto p2 = input.find(':', p1);
    auto p3 = input.find('"', p2);
    auto p4 = input.find('"', p3+1);
    metric = input.substr(p3+1, p4-p3-1);

    auto p5 = input.find("\"params\"");
    if (p5 != std::string::npos) {
        auto p6 = input.find('{', p5);
        auto p7 = input.find('}', p6);
        std::string blob = input.substr(p6+1, p7-p6-1);
        std::istringstream iss(blob);
        std::string tok;
        while (std::getline(iss, tok, ',')) {
            auto c1 = tok.find(':');
            if (c1 == std::string::npos) continue;
            std::string k = trim(tok.substr(0,c1));
            std::string v = trim(tok.substr(c1+1));
            if (!k.empty() && k.front()=='"' && k.back()=='"') k = k.substr(1,k.size()-2);
            bool ok=false;
            double d = to_double(v, ok);
            if (ok) params[k] = d;
        }
    }

    auto p8 = input.find("\"point\"");
    if (p8 != std::string::npos) {
        auto p9 = input.find('[', p8);
        auto p10 = input.find(']', p9);
        std::string blob = input.substr(p9+1, p10-p9-1);
        std::istringstream iss(blob);
        std::string tok;
        std::vector<double> vals;
        while (std::getline(iss, tok, ',')) {
            bool ok=false;
            double d = to_double(trim(tok), ok);
            if (ok) vals.push_back(d);
        }
        if (vals.size() >= 4) {
            t = vals[0]; x = vals[1]; y = vals[2]; z = vals[3];
        }
    }

    double r = std::sqrt(x*x + y*y + z*z);
    double theta = std::atan2(std::sqrt(x*x + y*y), z);
    double phi = std::atan2(y, x);

    if (metric == "schwarzschild") {
        double M = 1.0;
        auto it = params.find("M");
        if (it != params.end()) M = it->second;
        auto inv = schwarzschild_exact(M, r, theta, phi);
        if (!inv.valid) {
            std::cout << "{\"error\":\"singular or invalid point\"}\n";
            return true;
        }
        std::cout << "{"
                  << "\"metric\":\"schwarzschild\","
                  << "\"ricci_scalar\":" << json_number(inv.ricciScalar) << ","
                  << "\"kretschmann\":" << json_number(inv.kretschmann)
                  << "}\n";
    } else if (metric == "kerr") {
        double M = 1.0;
        auto itM = params.find("M");
        if (itM != params.end()) M = itM->second;
        double a = 0.5 * M;
        auto itA = params.find("a");
        if (itA != params.end()) a = itA->second;
        auto inv = kerr_exact(M, a, r, theta);
        if (!inv.valid) {
            std::cout << "{\"error\":\"singular or invalid point\"}\n";
            return true;
        }
        std::cout << "{"
                  << "\"metric\":\"kerr\","
                  << "\"ricci_scalar\":" << json_number(inv.ricciScalar) << ","
                  << "\"kretschmann\":" << json_number(inv.kretschmann)
                  << "}\n";
    } else if (metric == "flrw_matter") {
        double t0 = 1.0;
        auto it = params.find("t0");
        if (it != params.end()) t0 = it->second;
        auto inv = flrw_matter_exact(t, t0);
        if (!inv.valid) {
            std::cout << "{\"error\":\"singular or invalid point\"}\n";
            return true;
        }
        std::cout << "{"
                  << "\"metric\":\"flrw_matter\","
                  << "\"ricci_scalar\":" << json_number(inv.ricciScalar) << ","
                  << "\"kretschmann\":" << json_number(inv.kretschmann)
                  << "}\n";
    } else if (metric == "cghs") {
        double M = 1.0;
        auto itM = params.find("M");
        if (itM != params.end()) M = itM->second;
        double sigma = 0.0;
        auto itS = params.find("sigma");
        if (itS != params.end()) sigma = itS->second;
        auto inv = cghs_exact(M, sigma, t, x);
        if (!inv.valid) {
            std::cout << "{\"error\":\"singular or invalid point\"}\n";
            return true;
        }
        std::cout << "{"
                  << "\"metric\":\"cghs\","
                  << "\"ricci_scalar\":" << json_number(inv.ricciScalar) << ","
                  << "\"kretschmann\":" << json_number(inv.kretschmann)
                  << "}\n";
    } else {
        std::cout << "{\"error\":\"unsupported metric: " << json_escape(metric) << "\"}\n";
        return false;
    }
    return true;
}

} // namespace quantumverse

int main() {
    std::ios::sync_with_stdio(false);
    std::string line;
    bool first = true;
    while (std::getline(std::cin, line)) {
        line = quantumverse::trim(line);
        if (line.empty()) continue;
        if (!first) std::cout << "\n";
        first = false;
        quantumverse::handle(line);
    }
    return 0;
}
