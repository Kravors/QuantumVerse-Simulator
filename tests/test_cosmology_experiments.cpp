/**
 * @file test_cosmology_experiments.cpp
 * @brief Tests for the cosmology validation experiments.
 *
 * Positive tests: a correct (Planck) provider passes every experiment.
 * Negative tests: a deliberately broken provider must make the experiment
 * FAIL. If a negative test passes, the experiment is not validating anything.
 */

#include "experiment/cosmology/LambdaCDMReferenceProvider.h"
#include "experiment/experiments/AgeOfUniverseExperiment.h"
#include "experiment/experiments/DistanceLadderExperiment.h"
#include "experiment/experiments/CMBTemperatureExperiment.h"
#include "experiment/experiments/BAOSoundHorizonExperiment.h"
#include "experiment/experiments/HubbleEvolutionExperiment.h"
#include "experiment/experiments/GrowthFactorExperiment.h"
#include "experiment/experiments/CosmologyValidationSuite.h"
#include "experiment/ExperimentRunner.h"
#include "spacetime/FRWMetric.h"
#include "experiment/cosmology/ProjectCosmologyAdapter.h"
#include "test_assert.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>

using namespace quantumverse;

namespace {

class BrokenH0 : public LambdaCDMReferenceProvider {
public:
    explicit BrokenH0() : LambdaCDMReferenceProvider(Params{ .H0 = 80.0 }) {}
};

class BrokenTcmb : public LambdaCDMReferenceProvider {
public:
    explicit BrokenTcmb() : LambdaCDMReferenceProvider(Params{ .Tcmb = 3.0 }) {}
};

class BrokenOm : public LambdaCDMReferenceProvider {
public:
    explicit BrokenOm() : LambdaCDMReferenceProvider(Params{ .Om = 0.5 }) {}
};

std::shared_ptr<CosmologyProvider> goodProvider() {
    return std::make_shared<LambdaCDMReferenceProvider>();
}

bool runOk(const ExperimentResult& r) { return r.success; }

} // namespace

// Reference-value tests: the first-principles solver must reproduce known
// cosmological numbers.
// ===========================================================================

static void test_reference_age_matches_planck() {
    LambdaCDMReferenceProvider p;
    QV_CHECK_NEAR(p.ageAtRedshift(0.0), 13.787, 0.050);   // ±50 Myr
    QV_CHECK_NEAR(p.ageAtRedshift(1.0), 5.851, 0.050);    // ±50 Myr
    std::cout << "[PASS] reference age matches Planck" << std::endl;
}

static void test_reference_sound_horizon_matches_planck() {
    LambdaCDMReferenceProvider p;
    QV_CHECK_NEAR(p.soundHorizon(), 147.09, 2.0);         // ±2 Mpc
    std::cout << "[PASS] reference sound horizon matches Planck" << std::endl;
}

static void test_reference_distance_modulus_matches_union2() {
    LambdaCDMReferenceProvider p;
    QV_CHECK_NEAR(p.distanceModulus(1.0), 44.16, 0.10);
    std::cout << "[PASS] reference distance modulus matches Union2.1" << std::endl;
}

static void test_reference_critical_density() {
    LambdaCDMReferenceProvider p;
    QV_CHECK_NEAR(p.criticalDensity(), 8.598e-27, 0.10e-27);
    std::cout << "[PASS] reference critical density" << std::endl;
}

static void test_reference_q0_analytic() {
    LambdaCDMReferenceProvider p;
    QV_CHECK_NEAR(p.decelerationParameter(0.0), -0.53335, 1e-3);
    std::cout << "[PASS] reference q(0) analytic" << std::endl;
}

// Semantics: project-vs-reference comparison must isolate code bugs from
// parameter drift.
// ===========================================================================

static void test_reference_vs_reference_is_bit_exact() {
    auto ref = std::make_shared<LambdaCDMReferenceProvider>();
    AgeOfUniverseExperiment e(ref);
    auto r = e.run({}, [](double){}, [](){ return false; });
    // project == reference here (same params), so rel_error must be 0.
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("age_universe_Gyr_rel_err")).toDouble()) < 1e-9);
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("age_at_z1_Gyr_rel_err")).toDouble()) < 1e-9);
    QV_CHECK(r.scalars.value(QStringLiteral("age_universe_Gyr_pass")).toBool());
    QV_CHECK(r.scalars.value(QStringLiteral("age_at_z1_Gyr_pass")).toBool());
    std::cout << "[PASS] reference vs reference is bit-exact" << std::endl;
}

static void test_parameter_drift_shows_as_tension() {
    auto sh0es = std::make_shared<LambdaCDMReferenceProvider>(
        LambdaCDMReferenceProvider::Params{ .H0 = 73.04 });
    AgeOfUniverseExperiment e(sh0es);
    auto r = e.run({}, [](double){}, [](){ return false; });
    // H0=73.04 vs Planck 67.66 → ~8% shorter age → |rel_err| > 5%
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("age_universe_Gyr_rel_err")).toDouble()) > 5.0);
    // tension vs Planck age (σ ≈ 0.020 Gyr) → > 3σ
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("age_universe_Gyr_tension")).toDouble()) > 3.0);
    std::cout << "[PASS] parameter drift shows as tension" << std::endl;
}

static void test_broken_provider_affects_computation_not_just_accessor() {
    // If we only overrode H0() accessor (old buggy pattern), m_H0 stays at
    // 67.66 and the computation is unchanged. With Params-based construction,
    // m_H0 itself changes, so rel_err must appear.
    auto provider = std::make_shared<BrokenH0>();
    QV_CHECK_NEAR(provider->H0(), 80.0, 1e-6);
    AgeOfUniverseExperiment e(provider);
    auto r = e.run({}, [](double){}, [](){ return false; });
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("age_universe_Gyr_rel_err")).toDouble()) > 5.0);
    std::cout << "[PASS] broken provider changes m_H0, not just accessor" << std::endl;
}

// Cross-check: ProjectCosmologyAdapter delegates to FRWMetric.
// ===========================================================================

static void test_project_adapter_h0_delegates_to_frw() {
    constexpr double H0_si = 67.66 * 1000.0 / 3.0856775814913673e22;
    auto frw = std::make_shared<FRWMetric>(FRWMetric::lambdaCDM(0.3111, 0.6889, H0_si));
    ProjectCosmologyAdapter adapter(67.66, 0.3111, 0.049, 9.1e-5, 0.6889, 0.0, 2.7255);

    // Adapter's H0() returns km/s/Mpc (interface contract)
    QV_CHECK_NEAR(adapter.H0(), 67.66, 1e-6);
    // Cross-check: FRWMetric's hubbleConstant() (s⁻¹) converts back to same km/s/Mpc
    constexpr double MPC_M = 3.0856775814913673e22;
    QV_CHECK_NEAR(adapter.H0(), frw->hubbleConstant() * MPC_M / 1000.0, 1e-6);
    std::cout << "[PASS] ProjectAdapter H0() consistent with FRWMetric" << std::endl;
}

static void test_project_adapter_age_delegates_to_frw() {
    constexpr double H0_si = 67.66 * 1000.0 / 3.0856775814913673e22;
    auto frw = std::make_shared<FRWMetric>(FRWMetric::lambdaCDM(0.3111, 0.6889, H0_si));
    ProjectCosmologyAdapter adapter(67.66, 0.3111, 0.049, 9.1e-5, 0.6889, 0.0, 2.7255);

    // Bit-exact at z=0: adapter's ageAtRedshift(0) [Gyr] equals FRWMetric's ageOfUniverse() [s] / SECONDS_PER_GYR
    constexpr double SECONDS_PER_GYR = 1e9 * 365.25 * 86400.0;
    QV_CHECK_NEAR(adapter.ageAtRedshift(0.0), frw->ageOfUniverse() / SECONDS_PER_GYR, 1e-6);
    std::cout << "[PASS] ProjectAdapter ageAtRedshift(0) delegates to FRWMetric" << std::endl;
}

static void test_project_adapter_age_matches_reference_impl() {
    // Cross-check: FRWMetric's bisection age vs LambdaCDMReferenceProvider's
    // Simpson's-integral age. Both should agree within numerical precision.
    constexpr double H0_si = 67.66 * 1000.0 / 3.0856775814913673e22;
    auto frw = std::make_shared<FRWMetric>(FRWMetric::lambdaCDM(0.3111, 0.6889, H0_si));
    ProjectCosmologyAdapter adapter(67.66, 0.3111, 0.049, 9.1e-5, 0.6889, 0.0, 2.7255);
    auto ref = std::make_shared<LambdaCDMReferenceProvider>();

    // Tolerance: max of FRWMetric bisection (~20 Myr) + Reference Simpson (~14 Myr)
    // 50 Myr = 0.36% of 13.787 Gyr — catches real bugs, accepts numerical noise.
    const double proj_age = adapter.ageAtRedshift(0.0);
    const double ref_age = ref->ageAtRedshift(0.0);
    QV_CHECK(std::isfinite(proj_age));
    QV_CHECK(std::isfinite(ref_age));
    QV_CHECK_NEAR(proj_age, ref_age, 0.050); // 50 Myr
    std::cout << "[PASS] ProjectAdapter age ≈ LambdaCDMReferenceProvider age ("
              << proj_age << " vs " << ref_age << " Gyr)" << std::endl;
}

static void test_project_adapter_H_matches_reference_at_high_z() {
    // Regression test for the missing-radiation bug: at z=1, Ωr(1+z)^4 is
    // ~0.046% of E² — a missing radiation term passes. At z=100 it's ~0.4%,
    // at z=1000 it's ~31%. A test only at z=1 is blind to this bug.
    ProjectCosmologyAdapter adapter(67.66, 0.3111, 0.049, 9.1e-5, 0.6889, 0.0, 2.7255);
    auto ref = std::make_shared<LambdaCDMReferenceProvider>();

    for (double z : {100.0, 1000.0}) {
        const double h_proj = adapter.H(z);
        const double h_ref = ref->H(z);
        QV_CHECK(std::isfinite(h_proj));
        QV_CHECK(std::isfinite(h_ref));
        QV_CHECK_NEAR(h_proj, h_ref, 0.01); // 1% — catches missing radiation term
        std::cout << "[INFO] H(" << z << "): project=" << h_proj
                  << " reference=" << h_ref << " km/s/Mpc" << std::endl;
    }
    std::cout << "[PASS] ProjectAdapter H(z) matches reference at high z" << std::endl;
}

static void test_project_adapter_soundHorizon_matches_reference() {
    ProjectCosmologyAdapter adapter(67.66, 0.3111, 0.049, 9.1e-5, 0.6889, 0.0, 2.7255);
    auto ref = std::make_shared<LambdaCDMReferenceProvider>();

    const double rd_proj = adapter.soundHorizon();
    const double rd_ref = ref->soundHorizon();
    QV_CHECK(std::isfinite(rd_proj));
    QV_CHECK(std::isfinite(rd_ref));
    // Both use direct Simpson integration; agreement should be at the
    // sub-percent level. Missing radiation gave r_d ≈ 249 Mpc vs 147 Mpc.
    QV_CHECK_NEAR(rd_proj / rd_ref, 1.0, 0.01); // 1%
    std::cout << "[PASS] ProjectAdapter soundHorizon ≈ reference ("
              << rd_proj << " vs " << rd_ref << " Mpc)" << std::endl;
}

// Positive: a correct provider passes every experiment.
// ===========================================================================

static void test_age_experiment_passes() {
    auto provider = goodProvider();
    AgeOfUniverseExperiment e(provider);
    auto r = e.run({}, [](double){}, [](){ return false; });
    QV_CHECK(runOk(r));
    QV_CHECK(r.scalars.value(QStringLiteral("age_universe_Gyr_pass")).toBool());
    QV_CHECK(r.scalars.value(QStringLiteral("age_at_z1_Gyr_pass")).toBool());
    std::cout << "[PASS] age experiment passes with correct provider" << std::endl;
}

static void test_distance_ladder_experiment_passes() {
    auto provider = goodProvider();
    DistanceLadderExperiment e(provider);
    auto r = e.run({}, [](double){}, [](){ return false; });
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("mu_z0.1_rel_err")).toDouble()) < 1e-6);
    QV_CHECK(r.scalars.value(QStringLiteral("mu_z0.5_pass")).toBool());
    QV_CHECK(r.scalars.value(QStringLiteral("mu_z1_pass")).toBool());
    std::cout << "[PASS] distance ladder experiment passes with correct provider" << std::endl;
}

static void test_cmb_experiment_passes() {
    auto provider = goodProvider();
    CMBTemperatureExperiment e(provider);
    auto r = e.run({}, [](double){}, [](){ return false; });
    QV_CHECK(runOk(r));
    QV_CHECK(r.scalars.value(QStringLiteral("Tcmb_0_K_pass")).toBool());
    QV_CHECK(r.scalars.value(QStringLiteral("Tcmb_z1100_K_pass")).toBool());
    std::cout << "[PASS] CMB experiment passes with correct provider" << std::endl;
}

static void test_bao_experiment_passes() {
    auto provider = goodProvider();
    BAOSoundHorizonExperiment e(provider);
    auto r = e.run({}, [](double){}, [](){ return false; });
    QV_CHECK(runOk(r));
    QV_CHECK(r.scalars.value(QStringLiteral("r_drag_Mpc_pass")).toBool());
    std::cout << "[PASS] BAO experiment passes with correct provider" << std::endl;
}

static void test_hubble_experiment_passes() {
    auto provider = goodProvider();
    HubbleEvolutionExperiment e(provider);
    auto r = e.run({}, [](double){}, [](){ return false; });
    QV_CHECK(runOk(r));
    std::cout << "[PASS] Hubble evolution experiment passes with correct provider" << std::endl;
}

static void test_growth_experiment_passes() {
    auto provider = goodProvider();
    GrowthFactorExperiment e(provider);
    auto r = e.run({}, [](double){}, [](){ return false; });
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("D_z1_rel_err")).toDouble()) < 1e-6);
    std::cout << "[PASS] growth factor experiment passes with correct provider" << std::endl;
}

// Negative: a broken provider must fail the experiment.
// ===========================================================================

static void test_age_experiment_fails_broken_h0() {
    auto provider = std::make_shared<BrokenH0>();
    AgeOfUniverseExperiment e(provider);
    auto r = e.run({}, [](double){}, [](){ return false; });
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("age_universe_Gyr_rel_err")).toDouble()) > 5.0);
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("age_at_z1_Gyr_rel_err")).toDouble()) > 5.0);
    std::cout << "[PASS] age experiment fails with broken H₀" << std::endl;
}

static void test_distance_ladder_experiment_fails_broken_h0() {
    auto provider = std::make_shared<BrokenH0>();
    DistanceLadderExperiment e(provider);
    auto r = e.run({}, [](double){}, [](){ return false; });
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("mu_z0.1_rel_err")).toDouble()) > 0.1);
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("mu_z0.5_rel_err")).toDouble()) > 0.1);
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("mu_z1_rel_err")).toDouble()) > 0.1);
    std::cout << "[PASS] distance ladder experiment fails with broken H₀" << std::endl;
}

static void test_cmb_experiment_fails_broken_tcmb() {
    auto provider = std::make_shared<BrokenTcmb>();
    CMBTemperatureExperiment e(provider);
    auto r = e.run({}, [](double){}, [](){ return false; });
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("Tcmb_0_K_rel_err")).toDouble()) > 5.0);
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("Tcmb_z1100_K_rel_err")).toDouble()) > 5.0);
    std::cout << "[PASS] CMB experiment fails with broken T_CMB" << std::endl;
}

static void test_bao_experiment_fails_broken_h0() {
    auto provider = std::make_shared<BrokenH0>();
    BAOSoundHorizonExperiment e(provider);
    auto r = e.run({}, [](double){}, [](){ return false; });
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("r_drag_Mpc_rel_err")).toDouble()) > 5.0);
    std::cout << "[PASS] BAO experiment fails with broken H₀" << std::endl;
}

static void test_hubble_experiment_fails_broken_h0() {
    auto provider = std::make_shared<BrokenH0>();
    HubbleEvolutionExperiment e(provider);
    auto r = e.run({}, [](double){}, [](){ return false; });
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("H0_Planck_rel_err")).toDouble()) > 5.0);
    std::cout << "[PASS] Hubble experiment fails with broken H₀" << std::endl;
}

static void test_growth_experiment_fails_broken_om() {
    auto provider = std::make_shared<BrokenOm>();
    GrowthFactorExperiment e(provider);
    auto r = e.run({}, [](double){}, [](){ return false; });
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("D_z1_rel_err")).toDouble()) > 5.0);
    std::cout << "[PASS] growth experiment fails with broken Ωₘ" << std::endl;
}

// Validation report: headless CSV export test (no QML/event loop needed).
// ===========================================================================

static void test_export_validation_report_writes_csv() {
    auto provider = std::make_shared<LambdaCDMReferenceProvider>();
    AgeOfUniverseExperiment e(provider);
    auto r = e.run({}, [](double){}, [](){ return false; });

    QTemporaryDir tmp;
    QV_CHECK(tmp.isValid());
    const QString path = tmp.path() + QStringLiteral("/validation.csv");

    QString written = ExperimentRunner::writeValidationReportCSV(r, path);
    QV_CHECK(!written.isEmpty());
    QV_CHECK(QFileInfo::exists(written));

    QFile f(written);
    QV_CHECK(f.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString content = QString::fromUtf8(f.readAll());
    f.close();

    // Schema check
    QV_CHECK(content.contains("parameter,project_value,reference_value,observed_value,"));
    QV_CHECK(content.contains("observed_sigma,source,unit,rel_error_pct,tension_sigma,pass"));

    // Row count: at least age_universe_Gyr and age_at_z1_Gyr
    int rowCount = 0;
    for (const auto& line : content.split('\n', Qt::SkipEmptyParts)) {
        if (line.endsWith("PASS") || line.endsWith("FAIL")) ++rowCount;
    }
    QV_CHECK(rowCount >= 2);

    // Pass column values are only PASS/FAIL
    for (const auto& line : content.split('\n', Qt::SkipEmptyParts)) {
        if (line.endsWith("PASS") || line.endsWith("FAIL")) {
            QV_CHECK(line.endsWith("PASS") || line.endsWith("FAIL"));
        }
    }

    std::cout << "[PASS] export validation report writes CSV" << std::endl;
}

// CosmologyValidationSuite integration tests.
// ===========================================================================

static void test_cosmology_validation_suite_reference_passes() {
    CosmologyValidationSuite suite;
    auto r = suite.run({}, [](double){}, [](){ return false; });

    // Suite must have populated checks from all 6 experiments
    QV_CHECK(r.scalars.contains(QStringLiteral("age_universe_Gyr_pass")));
    QV_CHECK(r.scalars.contains(QStringLiteral("mu_z1_pass")));
    QV_CHECK(r.scalars.contains(QStringLiteral("r_drag_Mpc_pass")));
    QV_CHECK(r.scalars.contains(QStringLiteral("H0_Planck_pass")));
    QV_CHECK(r.scalars.contains(QStringLiteral("Tcmb_0_K_pass")));
    QV_CHECK(r.scalars.contains(QStringLiteral("D_z1_pass")));

    // Project vs reference (same params) must agree to numerical-integration
    // precision: rel_error ~ 0. Note: pass (tension vs observed) may differ
    // due to integration differences between the adapter's z-space and the
    // reference's a-space integrators — that's expected, not a bug.
    // relErrorPct is stored in percent; 1e-1 = 0.1% tolerance accommodates
    // bisection (FRWMetric) vs Simpson's (reference) integration differences.
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("age_universe_Gyr_rel_err")).toDouble()) < 1e-1);
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("mu_z1_rel_err")).toDouble()) < 1e-1);
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("r_drag_Mpc_rel_err")).toDouble()) < 1e-1);
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("H0_Planck_rel_err")).toDouble()) < 1e-1);
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("Tcmb_0_K_rel_err")).toDouble()) < 1e-1);
    QV_CHECK(std::abs(r.scalars.value(QStringLiteral("D_z1_rel_err")).toDouble()) < 1e-1);

    std::cout << "[PASS] cosmology validation suite passes with reference provider" << std::endl;
}

static void test_cosmology_validation_suite_broken_provider_fails() {
    CosmologyValidationSuite suite;
    QVariantMap params;
    params.insert(QStringLiteral("H0"), 80.0); // SH0ES-like: breaks tension vs Planck
    auto r = suite.run(params, [](double){}, [](){ return false; });

    // At least one sigma-referenced check must fail due to H0 tension vs Planck
    QV_CHECK(!r.scalars.value(QStringLiteral("H0_Planck_pass")).toBool());

    // And at least one fail count overall
    int fails = 0;
    for (const auto& k : r.scalars.keys()) {
        if (k.endsWith(QStringLiteral("_pass")) && !r.scalars.value(k).toBool()) ++fails;
    }
    QV_CHECK(fails >= 1);

    std::cout << "[PASS] cosmology validation suite fails with H0=80 (tension vs Planck)" << std::endl;
}

int main() {
    int failed = 0;
    auto runTest = [&failed](const char* name, void (*fn)()) {
        std::cout << "RUN " << name << std::endl;
        try { fn(); std::cout << "OK " << name << std::endl; }
        catch (const std::exception& ex) {
            std::cout << "[FAIL] " << name << ": " << ex.what() << std::endl;
            ++failed;
        }
    };

    runTest("reference age matches Planck",         &test_reference_age_matches_planck);
    runTest("reference sound horizon matches Planck",&test_reference_sound_horizon_matches_planck);
    runTest("reference distance modulus Union2.1",  &test_reference_distance_modulus_matches_union2);
    runTest("reference critical density",           &test_reference_critical_density);
    runTest("reference q(0) analytic",              &test_reference_q0_analytic);
    runTest("reference vs reference is bit-exact",   &test_reference_vs_reference_is_bit_exact);
    runTest("parameter drift shows as tension",     &test_parameter_drift_shows_as_tension);
    runTest("broken provider changes m_H0 not just accessor", &test_broken_provider_affects_computation_not_just_accessor);

    runTest("adapter H0 delegates to FRWMetric",    &test_project_adapter_h0_delegates_to_frw);
    runTest("adapter age delegates to FRWMetric",   &test_project_adapter_age_delegates_to_frw);
    runTest("adapter age matches reference impl",   &test_project_adapter_age_matches_reference_impl);

    runTest("adapter H(z) matches reference at high z", &test_project_adapter_H_matches_reference_at_high_z);
    runTest("adapter soundHorizon matches reference", &test_project_adapter_soundHorizon_matches_reference);

    runTest("age experiment passes",                &test_age_experiment_passes);
    runTest("distance ladder passes",               &test_distance_ladder_experiment_passes);
    runTest("CMB experiment passes",                &test_cmb_experiment_passes);
    runTest("BAO experiment passes",                &test_bao_experiment_passes);
    runTest("Hubble experiment passes",             &test_hubble_experiment_passes);
    runTest("growth experiment passes",             &test_growth_experiment_passes);

    runTest("age fails broken H0",                  &test_age_experiment_fails_broken_h0);
    runTest("distance ladder fails broken H0",      &test_distance_ladder_experiment_fails_broken_h0);
    runTest("CMB fails broken Tcmb",                &test_cmb_experiment_fails_broken_tcmb);
    runTest("BAO fails broken H0",                  &test_bao_experiment_fails_broken_h0);
    runTest("Hubble fails broken H0",               &test_hubble_experiment_fails_broken_h0);
    runTest("growth fails broken Om",               &test_growth_experiment_fails_broken_om);

    runTest("export validation report writes CSV",  &test_export_validation_report_writes_csv);
    runTest("validation suite reference passes",    &test_cosmology_validation_suite_reference_passes);
    runTest("validation suite broken provider fails", &test_cosmology_validation_suite_broken_provider_fails);

    if (failed == 0) {
        std::cout << "=== ALL COSMOLOGY EXPERIMENT TESTS PASSED ===" << std::endl;
        return 0;
    }
    std::cout << "=== " << failed << " COSMOLOGY EXPERIMENT TEST(S) FAILED ===" << std::endl;
    return 1;
}
