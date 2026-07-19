#include "discovery/TheoryDiscoveryAgent.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <cstdlib>

namespace quantumverse {
namespace discovery {

std::string escapeJson(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:   out += c; break;
        }
    }
    return out;
}

void emitJson(const TheoryDiscoveryAgent::DiscoveryResult& result, const std::string& theory_name) {
    std::cout << "{\n";
    std::cout << "  \"theory_name\": \"" << escapeJson(theory_name) << "\",\n";
    std::cout << "  \"total_reward\": " << std::setprecision(15) << result.total_reward << ",\n";
    std::cout << "  \"observational_chi2\": " << result.observational_chi2 << ",\n";
    std::cout << "  \"theoretical_penalty\": " << result.theoretical_penalty << ",\n";
    std::cout << "  \"simplicity_penalty\": " << result.simplicity_penalty << ",\n";
    std::cout << "  \"lorentzian_valid\": " << (result.lorentzian_valid ? "true" : "false") << ",\n";
    std::cout << "  \"kretschmann_nonneg\": " << (result.kretschmann_nonneg ? "true" : "false") << ",\n";
    std::cout << "  \"near_singularity\": " << (result.near_singularity ? "true" : "false") << ",\n";
    std::cout << "  \"parameters\": [";
    for (size_t i = 0; i < result.parameters.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << std::setprecision(15) << result.parameters[i];
    }
    std::cout << "]\n";
    std::cout << "}\n";
}

} // namespace discovery
} // namespace quantumverse

int main(int argc, char** argv) {
    using namespace quantumverse::discovery;

    int episodes = 50;
    std::string theory_str = "FR_GRAVITY";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--episodes" && i + 1 < argc) {
            episodes = std::atoi(argv[++i]);
        } else if (arg == "--theory" && i + 1 < argc) {
            theory_str = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cerr << "Usage: theory_discovery_runner [--episodes N] [--theory TYPE]\n";
            std::cerr << "  TYPE: FR_GRAVITY, BRANS_DICKE, LOOP_QUANTUM_GRAVITY\n";
            return 0;
        }
    }

    if (episodes <= 0) episodes = 50;

    TheoryParameterSpace::TheoryType theory_type = TheoryParameterSpace::TheoryType::FR_GRAVITY;
    if (theory_str == "BRANS_DICKE") {
        theory_type = TheoryParameterSpace::TheoryType::BRANS_DICKE;
    } else if (theory_str == "LOOP_QUANTUM_GRAVITY") {
        theory_type = TheoryParameterSpace::TheoryType::LOOP_QUANTUM_GRAVITY;
    }

    try {
        TheoryDiscoveryAgent agent(theory_type);
        agent.setSymbolicVerificationEnabled(true);
        agent.discoverBestTheory(episodes);
        const auto& best = agent.getBestResult();
        emitJson(best, theory_str);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "ERROR: unknown exception\n";
        return 1;
    }
}
