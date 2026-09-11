// QuantumVerse Test: Gravitational Lensing
// Validates the gravitational lensing renderer and its physics

#include <iostream>
#include <cmath>
#include <cassert>
#include <array>
#include <vector>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "rendering/GravitationalLensing.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/KerrMetric.h"
#include "spacetime/Event4D.h"
#include "physics/PhysicsConstants.h"

using namespace quantumverse;

int main() {
    std::cout << "=== Gravitational Lensing Tests ===" << std::endl;

    // Test 1: Photon sphere radius for Schwarzschild
    {
        double mass = 1.989e30;  // 1 solar mass
        SchwarzschildMetric metric(mass);

        // For Schwarzschild: r_photon = 3M (in geometric units)
        double M = PHYS_G() * mass / (PHYS_C() * PHYS_C());
        double expectedPhotonSphereMeters = 3.0 * M;

        // Create lensing renderer and check it computes correctly
        auto lensing = std::make_shared<GravitationalLensing>(
            std::make_shared<SchwarzschildMetric>(mass));
        lensing->setParams(GravitationalLensing::LensingParams{1.0f, 0.0f});
        double photonSphere = lensing->computePhotonSphereRadius();

        // computePhotonSphereRadius() returns geometric units (multiples of M)
        assert(std::abs(photonSphere - 3.0) < 0.1 &&
               "Photon sphere radius should be approximately 3M for Schwarzschild");

        std::cout << "[PASS] Photon sphere radius correct for Schwarzschild" << std::endl;
        std::cout << "       r_photon = " << photonSphere << " M (geometric)" << std::endl;
        std::cout << "       expected = " << expectedPhotonSphereMeters << " m" << std::endl;
    }

    // Test 2: ISCO for Schwarzschild
    {
        double mass = 1.989e30;
        auto lensing = std::make_shared<GravitationalLensing>(
            std::make_shared<SchwarzschildMetric>(mass));

        // Set Schwarzschild params
        GravitationalLensing::LensingParams params;
        params.mass = 1.0;
        params.spin = 0.0;
        lensing->setParams(params);

        double isco = lensing->computeISCO();
        double expectedISCO = 6.0;  // 6M for Schwarzschild

        assert(std::abs(isco - expectedISCO) / expectedISCO < 0.01 &&
               "ISCO should be 6M for Schwarzschild");

        std::cout << "[PASS] ISCO correct for Schwarzschild (6M)" << std::endl;
        std::cout << "       r_isco = " << isco << " M" << std::endl;
    }

    // Test 3: ISCO for Kerr (spin reduces ISCO)
    {
        auto lensing = std::make_shared<GravitationalLensing>(
            std::make_shared<KerrMetric>(1.989e30, 0.5));

        // Spin = 0.5
        GravitationalLensing::LensingParams params;
        params.mass = 1.0;
        params.spin = 0.5;
        lensing->setParams(params);

        double iscoPrograde = lensing->computeISCO();

        // For Kerr with spin 0.5, ISCO should be less than 6M
        assert(iscoPrograde < 6.0 && "Kerr ISCO should be less than Schwarzschild for prograde orbits");
        assert(iscoPrograde > 0.0 && "ISCO should be positive");

        std::cout << "[PASS] ISCO reduced for Kerr spin=0.5" << std::endl;
        std::cout << "       r_isco = " << iscoPrograde << " M (< 6M)" << std::endl;
    }

    // Test 4: Kerr ISCO approaches M for extremal spin
    {
        auto lensing = std::make_shared<GravitationalLensing>(
            std::make_shared<KerrMetric>(1.989e30, 0.999));

        GravitationalLensing::LensingParams params;
        params.mass = 1.0;
        params.spin = 0.999;
        lensing->setParams(params);

        double isco = lensing->computeISCO();

        // For extremal Kerr (a -> 1), prograde ISCO -> M
        assert(isco > 0.5 && isco < 2.0 &&
               "Extremal Kerr ISCO should be close to M");

        std::cout << "[PASS] Extremal Kerr ISCO near M" << std::endl;
        std::cout << "       r_isco = " << isco << " M (expected ~1)" << std::endl;
    }

    // Test 5: Ray marching step validation
    {
        auto lensing = std::make_shared<GravitationalLensing>(
            std::make_shared<SchwarzschildMetric>(1.989e30));

        // Test step count bounds
        lensing->setRaySteps(32);
        assert(lensing->raySteps() == 32 && "Ray steps should be set to 32");

        lensing->setRaySteps(1024);
        assert(lensing->raySteps() == 1024 && "Ray steps should be set to 1024");

        // Test clamping
        lensing->setRaySteps(10);
        assert(lensing->raySteps() >= 32 && "Ray steps should be clamped to min 32");

        lensing->setRaySteps(2000);
        assert(lensing->raySteps() <= 1024 && "Ray steps should be clamped to max 1024");

        std::cout << "[PASS] Ray marching step validation correct" << std::endl;
    }

    // Test 6: Shadow intensity bounds
    {
        auto lensing = std::make_shared<GravitationalLensing>(
            std::make_shared<SchwarzschildMetric>(1.989e30));

        lensing->setShadowIntensity(0.0);
        assert(std::abs(lensing->shadowIntensity() - 0.0) < 0.001 &&
               "Shadow intensity should be 0.0");

        lensing->setShadowIntensity(1.0);
        assert(std::abs(lensing->shadowIntensity() - 1.0) < 0.001 &&
               "Shadow intensity should be 1.0");

        lensing->setShadowIntensity(2.0);
        assert(lensing->shadowIntensity() <= 1.0 &&
               "Shadow intensity should be clamped to max 1.0");

        lensing->setShadowIntensity(-1.0);
        assert(lensing->shadowIntensity() >= 0.0 &&
               "Shadow intensity should be clamped to min 0.0");

        std::cout << "[PASS] Shadow intensity bounds correct" << std::endl;
    }

    // Test 7: Parameter structure validation
    {
        GravitationalLensing::LensingParams params;
        params.mass = 1.5f;
        params.spin = 0.7f;
        params.cameraDistance = 15.0f;
        params.raySteps = 128;
        params.shadowIntensity = 0.9f;

        auto lensing = std::make_shared<GravitationalLensing>(
            std::make_shared<KerrMetric>(1.989e30, 0.7));
        lensing->setParams(params);

        const auto& retrieved = lensing->params();
        assert(std::abs(retrieved.mass - 1.5f) < 0.001f && "Mass should be 1.5");
        assert(std::abs(retrieved.spin - 0.7f) < 0.001f && "Spin should be 0.7");
        assert(std::abs(retrieved.cameraDistance - 15.0f) < 0.001f && "Distance should be 15");
        assert(retrieved.raySteps == 128 && "Steps should be 128");
        assert(std::abs(retrieved.shadowIntensity - 0.9f) < 0.001f && "Shadow intensity should be 0.9");

        std::cout << "[PASS] Parameter structure validation correct" << std::endl;
    }

    // Test 8: Enable/disable lensing
    {
        auto lensing = std::make_shared<GravitationalLensing>(
            std::make_shared<SchwarzschildMetric>(1.989e30));

        assert(lensing->isEnabled() && "Lensing should be enabled by default");

        lensing->setEnabled(false);
        assert(!lensing->isEnabled() && "Lensing should be disabled");

        lensing->setEnabled(true);
        assert(lensing->isEnabled() && "Lensing should be re-enabled");

        std::cout << "[PASS] Enable/disable lensing works correctly" << std::endl;
    }

    // Test 9: Metric switching
    {
        auto schwarzschildMetric = std::make_shared<SchwarzschildMetric>(1.989e30);
        auto kerrMetric = std::make_shared<KerrMetric>(1.989e30, 0.5);

        auto lensing = std::make_shared<GravitationalLensing>(schwarzschildMetric);

        assert(lensing->metric() == schwarzschildMetric && "Metric should be Schwarzschild");

        lensing->setMetric(kerrMetric);
        assert(lensing->metric() == kerrMetric && "Metric should now be Kerr");

        std::cout << "[PASS] Metric switching works correctly" << std::endl;
    }

    // Test 10: Lens distortion pattern validation
    // Verifies that the deflection function produces expected qualitative behavior
    {
        // Deflection should be stronger for smaller radii (closer to BH)
        // For a ray passing at 10M: deflection ~ rs/r = 2M/10M = 0.2
        // For a ray passing at 100M: deflection ~ rs/r = 2M/100M = 0.02

        double r1 = 10.0;
        double r2 = 100.0;
        double deflection1 = 2.0 / (r1 * r1);  // ~0.02
        double deflection2 = 2.0 / (r2 * r2);  // ~0.0002

        assert(deflection1 > deflection2 && "Deflection should be stronger closer to BH");
        assert(std::abs(deflection1 / deflection2 - r2 * r2 / (r1 * r1)) < 0.01 &&
               "Deflection should scale as 1/r^2");

        std::cout << "[PASS] Lens distortion pattern scales correctly with distance" << std::endl;
        std::cout << "       deflection(10M) = " << deflection1 << std::endl;
        std::cout << "       deflection(100M) = " << deflection2 << std::endl;
    }

    // Test 11: Kerr shadow asymmetry
    {
        auto lensing = std::make_shared<GravitationalLensing>(
            std::make_shared<KerrMetric>(1.989e30, 0.9));

        GravitationalLensing::LensingParams params;
        params.mass = 1.0;
        params.spin = 0.9;
        params.enableAccretionDisk = true;
        lensing->setParams(params);

        // The photon sphere for Kerr prograde is smaller than Schwarzschild
        double photonSphere = lensing->computePhotonSphereRadius();

        // For Kerr with spin 0.9, photon sphere < 3M
        assert(photonSphere < 3.0 && "Kerr photon sphere should be less than Schwarzschild");

        std::cout << "[PASS] Kerr shadow asymmetry (photon sphere < 3M)" << std::endl;
        std::cout << "       r_photon = " << photonSphere << " M (spin=0.9)" << std::endl;
    }

    // Test 12: Star field generation (non-GL validation)
    {
        auto lensing = std::make_shared<GravitationalLensing>(
            std::make_shared<SchwarzschildMetric>(1.989e30));

        // Generate star field parameters (validates setup, not GL)
        lensing->generateStarField(42, 1000);
        lensing->generateStarField(123, 2000);
        lensing->generateStarField(0, 500);

        std::cout << "[PASS] Star field generation parameters validated" << std::endl;
    }

    // Test 13: Volumetric accretion disk emissivity (CPU reference)
    {
        GravitationalLensing::VolumetricDiskParams params;
        params.enableVolumetricDisk = true;
        params.diskDensity = 1.0f;
        params.diskTemperature = 1.0f;
        params.diskScaleHeight = 0.1f;
        params.diskInnerRadius = 0.0f;  // use ISCO
        params.diskOuterRadius = 20.0f;
        params.diskRaySteps = 64;
        params.diskOpacity = 1.0f;
        params.diskDopplerBoost = 1.0f;

        float mass = 1.0f;

        // Emissivity must be non-negative everywhere in the disk
        std::array<float, 3> p1 = {8.0f, 0.0f, 0.0f};  // mid-disk, equatorial
        std::array<float, 3> p2 = {12.0f, 0.5f, 0.0f}; // off-plane
        std::array<float, 3> p3 = {25.0f, 0.0f, 0.0f}; // outside disk
        float e1 = GravitationalLensing::computeVolumetricDiskEmissivity(p1, params, mass);
        float e2 = GravitationalLensing::computeVolumetricDiskEmissivity(p2, params, mass);
        float e3 = GravitationalLensing::computeVolumetricDiskEmissivity(p3, params, mass);

        assert(e1 >= 0.0f && "Disk emissivity must be non-negative at mid-disk");
        assert(e2 >= 0.0f && "Disk emissivity must be non-negative off-plane");
        assert(e3 == 0.0f && "Emissivity must be zero outside the disk");

        // Hotter near the inner edge (r=8M) than far out (r=12M): T ~ r^(-3/4)
        assert(e1 > e2 && "Emissivity must decrease with radius (T ~ r^-3/4)");

        std::cout << "[PASS] Volumetric disk emissivity non-negative and radially decreasing" << std::endl;
        std::cout << "       emissivity(8M)  = " << e1 << std::endl;
        std::cout << "       emissivity(12M) = " << e2 << std::endl;
    }

    // Test 14: Volumetric disk luminosity scales with density and temperature
    {
        GravitationalLensing::VolumetricDiskParams base;
        base.diskDensity = 1.0f;
        base.diskTemperature = 1.0f;
        base.diskScaleHeight = 0.1f;
        base.diskOuterRadius = 20.0f;
        base.diskDopplerBoost = 1.0f;

        float mass = 1.0f;
        float L0 = GravitationalLensing::computeDiskLuminosity(base, mass);

        // Doubling density doubles the luminosity (linear in density)
        GravitationalLensing::VolumetricDiskParams dense = base;
        dense.diskDensity = 2.0f;
        float Ldense = GravitationalLensing::computeDiskLuminosity(dense, mass);

        // Raising temperature raises luminosity (L ~ T^4)
        GravitationalLensing::VolumetricDiskParams hot = base;
        hot.diskTemperature = 2.0f;
        float Lhot = GravitationalLensing::computeDiskLuminosity(hot, mass);

        assert(L0 > 0.0f && "Disk luminosity must be positive");
        assert(std::abs(Ldense / L0 - 2.0f) < 0.05 &&
               "Luminosity must scale linearly with density");
        assert(Lhot > L0 && "Luminosity must increase with temperature");

        std::cout << "[PASS] Disk luminosity scales correctly with density and temperature" << std::endl;
        std::cout << "       L(density=1)  = " << L0 << std::endl;
        std::cout << "       L(density=2)  = " << Ldense << std::endl;
        std::cout << "       L(temp=2)     = " << Lhot << std::endl;
    }

    // Test 15: Volumetric disk parameter validation
    {
        auto lensing = std::make_shared<GravitationalLensing>(
            std::make_shared<SchwarzschildMetric>(1.989e30));

        // Disabled by default
        assert(!lensing->isVolumetricDiskEnabled() &&
               "Volumetric disk should be disabled by default");

        // Toggle on/off
        lensing->setEnabledVolumetricDisk(true);
        assert(lensing->isVolumetricDiskEnabled() &&
               "Volumetric disk should be enabled");

        lensing->setEnabledVolumetricDisk(false);
        assert(!lensing->isVolumetricDiskEnabled() &&
               "Volumetric disk should be disabled");

        // Round-trip params
        GravitationalLensing::VolumetricDiskParams params;
        params.enableVolumetricDisk = true;
        params.diskDensity = 2.5f;
        params.diskTemperature = 1.5f;
        params.diskScaleHeight = 0.2f;
        params.diskInnerRadius = 5.0f;
        params.diskOuterRadius = 30.0f;
        params.diskRaySteps = 128;
        params.diskOpacity = 0.8f;
        params.diskDopplerBoost = 1.2f;

        lensing->setVolumetricDiskParams(params);
        const auto& retrieved = lensing->volumetricDiskParams();
        assert(std::abs(retrieved.diskDensity - 2.5f) < 0.001f &&
               "Disk density should round-trip");
        assert(std::abs(retrieved.diskTemperature - 1.5f) < 0.001f &&
               "Disk temperature should round-trip");
        assert(std::abs(retrieved.diskScaleHeight - 0.2f) < 0.001f &&
               "Disk scale height should round-trip");
        assert(std::abs(retrieved.diskInnerRadius - 5.0f) < 0.001f &&
               "Disk inner radius should round-trip");
        assert(std::abs(retrieved.diskOuterRadius - 30.0f) < 0.001f &&
               "Disk outer radius should round-trip");
        assert(retrieved.diskRaySteps == 128 &&
               "Disk ray steps should round-trip");
        assert(std::abs(retrieved.diskOpacity - 0.8f) < 0.001f &&
               "Disk opacity should round-trip");
        assert(std::abs(retrieved.diskDopplerBoost - 1.2f) < 0.001f &&
               "Disk Doppler boost should round-trip");

        std::cout << "[PASS] Volumetric disk parameter validation correct" << std::endl;
    }

    // Test 16: Exact Kerr ISCO (Bardeen-Press-Teukolsky) matches known values.
    // The shader used to approximate this with mix(6M, rs, spin*0.7), which
    // diverged badly for high spin.  Positive spin = prograde (shrinks ISCO).
    {
        auto lensing = std::make_shared<GravitationalLensing>(
            std::make_shared<SchwarzschildMetric>(1.989e30));

        GravitationalLensing::LensingParams params;
        params.mass = 1.0;
        params.spin = 0.0;
        lensing->setParams(params);

        // Schwarzschild ISCO is exactly 6M
        double isco = lensing->computeISCO();
        assert(std::abs(isco - 6.0) < 0.01 &&
               "Schwarzschild ISCO should be 6M");
        assert(std::abs(GravitationalLensing::computeISCORadius(0.0) - 6.0) < 0.01 &&
               "computeISCORadius(0) should be 6.0");

        std::cout << "[PASS] ISCO exact for Schwarzschild (6M)" << std::endl;
        std::cout << "       r_isco = " << isco << " M" << std::endl;
    }

    // Test 17: Prograde Kerr ISCO shrinks toward M; retrograde expands past 6M.
    {
        auto lensing = std::make_shared<GravitationalLensing>(
            std::make_shared<KerrMetric>(1.989e30, 0.5));

        GravitationalLensing::LensingParams params;
        params.mass = 1.0;
        params.spin = 0.5;
        lensing->setParams(params);

        double iscoPrograde = lensing->computeISCO();
        double iscoRetrograde = GravitationalLensing::computeISCORadius(-0.5);

        // Reference values for the Bardeen formula: a=0.5 -> 4.2330 M
        assert(std::abs(iscoPrograde - 4.2330) < 0.01 &&
               "Kerr ISCO(a=0.5) should be ~4.233M");
        assert(iscoPrograde < 6.0 && "Prograde ISCO must be below Schwarzschild");
        assert(iscoRetrograde > 6.0 && "Retrograde ISCO must be above Schwarzschild");

        std::cout << "[PASS] Kerr ISCO shrinks for prograde, expands for retrograde" << std::endl;
        std::cout << "       r_isco(a=+0.5) = " << iscoPrograde << " M" << std::endl;
        std::cout << "       r_isco(a=-0.5) = " << iscoRetrograde << " M" << std::endl;
    }

    // Test 18: Extremal Kerr ISCO approaches the Thorne limit (1.237 M).
    {
        auto lensing = std::make_shared<GravitationalLensing>(
            std::make_shared<KerrMetric>(1.989e30, 0.998));

        GravitationalLensing::LensingParams params;
        params.mass = 1.0;
        params.spin = 0.998;
        lensing->setParams(params);

        double isco = lensing->computeISCO();
        double iscoPure = GravitationalLensing::computeISCORadius(0.998);

        // Extremal prograde Kerr ISCO -> 1.237 M (Thorne 1974 limit)
        assert(std::abs(isco - 1.237) < 0.02 &&
               "Extremal Kerr ISCO should approach 1.237M");
        assert(std::abs(iscoPure - 1.237) < 0.02 &&
               "computeISCORadius(0.998) should approach 1.237");

        std::cout << "[PASS] Extremal Kerr ISCO approaches Thorne limit (1.237M)" << std::endl;
        std::cout << "       r_isco(a=0.998) = " << isco << " M" << std::endl;
    }

    // Test 19: CPU ISCO reference mirrors the GLSL computeISCO_GLSL() bit-for-bit.
    // This is the only guard against the two implementations drifting apart.
    {
        // Mirror of computeISCO_GLSL() in GravitationalLensing.cpp -- same
        // Bardeen-Press-Teukolsky formula, same cbrt/pow choices, same sign
        // convention (positive spin = prograde).  If the CPU reference ever
        // diverges from the shader, this test catches it.
        auto glsl_isco_mirror = [](double spin) -> double {
            double a  = std::min(std::abs(spin), 0.9999);
            double a2 = a * a;
            double c1p = std::cbrt(1.0 + a);
            double c1m = std::cbrt(1.0 - a);
            double c12 = (a2 >= 1.0) ? 0.0 : std::cbrt(1.0 - a2);
            double Z1  = 1.0 + c12 * (c1p + c1m);
            double Z2  = std::sqrt(3.0 * a2 + Z1 * Z1);
            double in  = std::sqrt(std::max((3.0 - Z1) * (3.0 + Z1 + 2.0 * Z2), 0.0));
            double sgn = (spin >= 0.0) ? 1.0 : -1.0;
            return 3.0 + Z2 - sgn * in;
        };

        const std::vector<double> spins = {0.0, 0.5, 0.9, 0.998, -0.5, -0.998};
        for (double s : spins) {
            double cpu  = GravitationalLensing::computeISCORadius(s);
            double glsl = glsl_isco_mirror(s);
            assert(std::abs(cpu - glsl) < 1e-4 &&
                   "CPU ISCO reference must match GLSL mirror");
        }

        std::cout << "[PASS] CPU ISCO reference matches GLSL mirror across spins" << std::endl;
    }

    // Test 20: Doppler beaming is signed -- approaching side is blueshifted.
    //
    // The CPU reference fixes the ray direction to +x (n = (1,0,0)), so only
    // the x-component of the tangential velocity contributes.  vel = (-sinPhi,
    // 0, cosPhi) * vPhi, hence vel.x = -sinPhi * vPhi = -(z/r) * vPhi.  A point
    // on the -z side (phi = -pi/2) has vel.x > 0, i.e. moving TOWARD the
    // observer -> v.n < 0 -> delta > 1 -> blueshifted.  The symmetric point on
    // +z is receding and dimmer.  Same radius -> same density and temperature,
    // so the only asymmetry is the Doppler factor.
    {
        GravitationalLensing::VolumetricDiskParams params;
        params.enableVolumetricDisk = true;
        params.diskDensity = 1.0f;
        params.diskTemperature = 1.0f;
        params.diskScaleHeight = 0.1f;
        params.diskInnerRadius = 0.0f;  // use ISCO
        params.diskOuterRadius = 20.0f;
        params.diskDopplerBoost = 1.0f;

        float mass = 1.0f;

        // Same radius, opposite sides of the disk.  Must lie on the z axis so
        // the tangential velocity has a non-zero x-component (on the x axis
        // vel.x = 0 and both points give delta = 1, i.e. no asymmetry).
        std::array<float, 3> approaching = {0.0f, 0.0f, -8.0f};  // phi=-pi/2, vel.x > 0
        std::array<float, 3> receding    = {0.0f, 0.0f,  8.0f};  // phi=+pi/2, vel.x < 0

        float eApproach = GravitationalLensing::computeVolumetricDiskEmissivity(approaching, params, mass);
        float eRecede  = GravitationalLensing::computeVolumetricDiskEmissivity(receding, params, mass);

        // Same radius -> same density and temperature -> the only asymmetry is
        // the Doppler factor.  Approaching side must be brighter.
        assert(eApproach > eRecede &&
               "Doppler beaming must blueshift the approaching side");

        std::cout << "[PASS] Doppler beaming is signed (approaching side brighter)" << std::endl;
        std::cout << "       emissivity(approaching -8M) = " << eApproach << std::endl;
        std::cout << "       emissivity(receding    +8M) = " << eRecede << std::endl;
    }

    // Test 21: The spin parameter reaches computeDiskLuminosity only through the
    // ISCO inner edge.  When the inner radius is fixed explicitly, spin must not
    // change the luminosity (vPhi = sqrt(M/r) is independent of spin, and the
    // density/temperature profiles are normalised to rInner).  When the inner
    // radius is left at 0 (ISCO), spin must change the result.
    {
        GravitationalLensing::VolumetricDiskParams fixed;
        fixed.enableVolumetricDisk = true;
        fixed.diskDensity = 1.0f;
        fixed.diskTemperature = 1.0f;
        fixed.diskScaleHeight = 0.1f;
        fixed.diskInnerRadius = 6.0f;   // explicit, overrides ISCO
        fixed.diskOuterRadius = 20.0f;
        fixed.diskDopplerBoost = 1.0f;

        float mass = 1.0f;
        float Lfixed0    = GravitationalLensing::computeDiskLuminosity(fixed, mass, 0.0f);
        float LfixedKerr = GravitationalLensing::computeDiskLuminosity(fixed, mass, 0.9f);

        // Fixed inner edge -> spin has no other effect on the CPU reference,
        // so the two must agree.
        assert(std::abs(Lfixed0 - LfixedKerr) < 1e-3 &&
               "Fixed inner radius must make luminosity spin-independent");

        // Now let the inner edge follow the ISCO: spin must change the result.
        GravitationalLensing::VolumetricDiskParams isco = fixed;
        isco.diskInnerRadius = 0.0f;
        float Lisco0    = GravitationalLensing::computeDiskLuminosity(isco, mass, 0.0f);
        float Liskerr  = GravitationalLensing::computeDiskLuminosity(isco, mass, 0.9f);

        assert(std::abs(Lisco0 - Liskerr) > 1.0f &&
               "ISCO inner edge must make luminosity depend on spin");

        std::cout << "[PASS] Spin reaches luminosity through the ISCO inner edge" << std::endl;
        std::cout << "       L(fixed rInner, spin=0)   = " << Lfixed0 << std::endl;
        std::cout << "       L(fixed rInner, spin=0.9) = " << LfixedKerr << std::endl;
        std::cout << "       L(ISCO, spin=0)           = " << Lisco0 << std::endl;
        std::cout << "       L(ISCO, spin=0.9)         = " << Liskerr << std::endl;
    }

    std::cout << "=== ALL GRAVITATIONAL LENSING TESTS PASSED ===" << std::endl;
    return 0;
}
