#include <cmath>
#include <limits>
#include "bayesian/BayesianEvidenceCalculator.h"
#include "test_assert.h"

using namespace quantumverse;

int main() {
    // ------------------------------------------------------------------
    // Test 1 (SIGN CONVENTION — non-vacuous).
    // ref: k=3, n=100, logLik=-50   → BIC ≈ 113.816
    // cand: k=3, n=100, logLik=-45  → BIC ≈ 103.816 (candidate fits better)
    // logBF = (BIC_ref − BIC_cand)/2 = (113.816 − 103.816)/2 = +5.0
    // logBF > 0 → candidate favored. Catches sign flip (±ΔBIC/2).
    {
        BICInputs refIn{3, 100, -50.0};
        BICInputs candIn{3, 100, -45.0};
        auto r = BayesianEvidenceCalculator::compareFromInputs("ref", refIn, "cand", candIn);
        QV_CHECK(r.logBF_natural > 0.0);  // candidate favored
        QV_CHECK_NEAR(r.logBF_natural, 5.0, 1e-9);
    }

    // Test 2 (SIGN CONVENTION — mirror of Test 1).
    // Swap ref/cand → sign flips → logBF = −5.0, reference favored.
    {
        BICInputs refIn{3, 100, -45.0};   // BIC ≈ 103.816 (better fit)
        BICInputs candIn{3, 100, -50.0};  // BIC ≈ 113.816 (worse fit)
        auto r = BayesianEvidenceCalculator::compareFromInputs("ref", refIn, "cand", candIn);
        QV_CHECK(r.logBF_natural < 0.0);  // reference favored
        QV_CHECK_NEAR(r.logBF_natural, -5.0, 1e-9);
    }

    // Test 3 (IDENTICAL MODELS → BF = 1, logBF = 0).
    {
        BICInputs a{4, 200, -100.0};
        auto r = BayesianEvidenceCalculator::compareFromInputs("A", a, "B", a);
        QV_CHECK_NEAR(r.logBF_natural, 0.0, 1e-12);
        QV_CHECK(r.interpretation == "inconclusive");
    }

    // Test 4 (NON-VACUOUS — equal ΔlogLik, growing n must NOT change BF).
    // k equal → k·ln(n) cancels in ΔBIC → same logBF regardless of n.
    // Catches the "forgot k·ln(n) cancellation" bug.
    {
        BICInputs refSmall{3, 100, -50.0};
        BICInputs candSmall{3, 100, -45.0};
        auto rSmall = BayesianEvidenceCalculator::compareFromInputs("r", refSmall, "c", candSmall);

        BICInputs refLarge{3, 10000, -5000.0};
        BICInputs candLarge{3, 10000, -4995.0};
        auto rLarge = BayesianEvidenceCalculator::compareFromInputs("r", refLarge, "c", candLarge);

        QV_CHECK_NEAR(rSmall.logBF_natural, 5.0, 1e-9);
        QV_CHECK_NEAR(rLarge.logBF_natural, 5.0, 1e-9);
    }

    // Test 5 (NON-VACUOUS — parameter penalty, all else equal).
    // cand has 2 more params at same logLik → BIC_cand higher by Δk·ln(n)
    // logBF = −Δk·ln(n)/2 = −2·ln(100)/2 = −ln(100) ≈ −4.605
    // Reference (simpler) favored.
    {
        const int n = 100;
        BICInputs ref{3, n, -50.0};
        BICInputs cand{5, n, -50.0};
        auto r =
            BayesianEvidenceCalculator::compareFromInputs("parsimonious", ref, "complex", cand);
        QV_CHECK(r.logBF_natural < 0.0);  // simpler model favored
        QV_CHECK_NEAR(r.logBF_natural, -4.605, 0.01);
    }

    // Test 6 (INTERPRETATION thresholds — Kass & Raftery on |ln BF|).
    {
        QV_CHECK(BayesianEvidenceCalculator::interpret(0.5) == "inconclusive");
        QV_CHECK(BayesianEvidenceCalculator::interpret(2.0) == "positive");
        QV_CHECK(BayesianEvidenceCalculator::interpret(4.0) == "strong");
        QV_CHECK(BayesianEvidenceCalculator::interpret(6.0) == "very strong");
        QV_CHECK(BayesianEvidenceCalculator::interpret(-6.0) == "very strong");  // abs
        QV_CHECK(BayesianEvidenceCalculator::interpret(std::numeric_limits<double>::quiet_NaN()) ==
                 "invalid");
    }

    return 0;
}
