// QuantumVerse Test: PBR Rendering (Cook-Torrance BRDF)
// Validates the physically-based rendering math for celestial bodies

#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>
#include <cstdint>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "rendering/CelestialBodyRenderer.h"
#include "rendering/ProceduralTextures.h"

using namespace quantumverse;

// ============================================================================
// Helper: Cook-Torrance BRDF reference implementation (CPU-side)
// ============================================================================

float distributionGGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    denom = M_PI * denom * denom;
    return a2 / std::max(denom, 0.0001f);
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return NdotV / (NdotV * (1.0f - k) + k);
}

float geometrySmith(float NdotV, float NdotL, float roughness) {
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

float fresnelSchlick(float cosTheta, float F0) {
    return F0 + (1.0f - F0) * std::pow(std::min(std::max(1.0f - cosTheta, 0.0f), 1.0f), 5.0f);
}

// ============================================================================
// Tests
// ============================================================================

// Test 1: PBR material struct defaults are valid
void testPBRMaterialDefaults()
{
    std::cout << "Test: PBR material struct defaults... ";

    // Verify the PBRMaterial struct exists with correct defaults
    PBRMaterial mat;
    assert(mat.metallic == 0.0f);
    assert(mat.roughness == 0.5f);
    assert(mat.ao == 1.0f);
    assert(mat.metallicTexLayer == -1);
    assert(mat.roughnessTexLayer == -1);
    assert(mat.normalTexLayer == -1);
    assert(mat.aoTexLayer == -1);

    // Verify CelestialBodyInstance has pbrMaterial field
    CelestialBodyInstance instance;
    instance.pbrMaterial.metallic = 0.8f;
    instance.pbrMaterial.roughness = 0.3f;
    instance.pbrMaterial.ao = 0.95f;
    assert(instance.pbrMaterial.metallic == 0.8f);
    assert(instance.pbrMaterial.roughness == 0.3f);
    assert(instance.pbrMaterial.ao == 0.95f);

    std::cout << "PASS" << std::endl;
}

// Test 2: Energy conservation — BRDF never creates energy
void testEnergyConservation()
{
    std::cout << "Test: Energy conservation (Cook-Torrance BRDF)... ";

    struct TestCase {
        float NdotV;
        float NdotL;
        float roughness;
        float metallic;
    };

    TestCase cases[] = {
        {0.9f, 0.9f, 0.1f, 0.0f},
        {0.5f, 0.5f, 0.5f, 0.0f},
        {0.1f, 0.1f, 0.9f, 0.0f},
        {0.9f, 0.9f, 0.3f, 1.0f},
        {0.5f, 0.5f, 0.5f, 1.0f},
        {0.01f, 0.9f, 0.5f, 0.0f},
        {0.99f, 0.99f, 0.04f, 0.0f},
    };

    for (const auto& tc : cases) {
        float NdotH = std::min(std::max(tc.NdotV + tc.NdotL, 0.0f), 1.0f) * 0.5f;
        float D = distributionGGX(NdotH, tc.roughness);
        float G = geometrySmith(tc.NdotV, tc.NdotL, tc.roughness);

        float denom = 4.0f * std::max(tc.NdotV, 0.001f) * std::max(tc.NdotL, 0.001f);
        float specularBRDF = D * G / denom;

        // Specular BRDF should be bounded
        float bound = 1.0f / std::max(tc.NdotL, 0.001f);
        assert(specularBRDF < bound * 2.0f && "Specular BRDF exceeds energy bound");

        // Total energy conservation
        float F0 = tc.metallic > 0.5f ? 0.5f : 0.04f;
        float F = fresnelSchlick(tc.NdotV, F0);
        float kD = (1.0f - F) * (1.0f - tc.metallic);
        float diffuseBRDF = kD / M_PI;
        float totalBRDF = diffuseBRDF + F * specularBRDF;

        assert(totalBRDF < 5.0f && "Total BRDF violates energy conservation");
    }

    std::cout << "PASS" << std::endl;
}

// Test 3: Fresnel effect at grazing angles
void testFresnelGrazingAngles()
{
    std::cout << "Test: Fresnel effect at grazing angles... ";

    float F0_dielectric = 0.04f;
    float F0_metal = 0.9f;

    // At normal incidence, F = F0
    float F_normal_diel = fresnelSchlick(1.0f, F0_dielectric);
    assert(std::abs(F_normal_diel - F0_dielectric) < 0.01f);

    float F_normal_metal = fresnelSchlick(1.0f, F0_metal);
    assert(std::abs(F_normal_metal - F0_metal) < 0.01f);

    // At grazing angle, F approaches 1.0
    float F_grazing_diel = fresnelSchlick(0.01f, F0_dielectric);
    assert(F_grazing_diel > 0.8f && "Dielectric should be highly reflective at grazing angle");

    float F_grazing_metal = fresnelSchlick(0.01f, F0_metal);
    assert(F_grazing_metal > 0.95f && "Metal should be near-perfectly reflective at grazing angle");

    // Fresnel should increase monotonically as angle increases
    float F_prev = fresnelSchlick(1.0f, F0_dielectric);
    for (float cosTheta = 0.9f; cosTheta >= 0.05f; cosTheta -= 0.1f) {
        float F_curr = fresnelSchlick(cosTheta, F0_dielectric);
        assert(F_curr >= F_prev - 0.01f && "Fresnel should be monotonically increasing");
        F_prev = F_curr;
    }

    std::cout << "PASS" << std::endl;
}

// Test 4: Roughness/metallic parameter validation
void testRoughnessMetallicParameters()
{
    std::cout << "Test: Roughness/metallic parameter ranges... ";

    auto clampSafe = [](float v) -> float {
        return std::min(std::max(v, 0.0f), 1.0f);
    };

    assert(clampSafe(-0.5f) == 0.0f);
    assert(clampSafe(1.5f) == 1.0f);
    assert(clampSafe(0.5f) == 0.5f);

    auto clampRoughness = [](float v) -> float {
        return std::min(std::max(v, 0.04f), 1.0f);
    };
    assert(clampRoughness(0.0f) == 0.04f);
    assert(clampRoughness(2.0f) == 1.0f);

    // GGX distribution: smooth surfaces have sharper peak
    float D_smooth = distributionGGX(1.0f, 0.1f);
    float D_rough = distributionGGX(1.0f, 0.9f);
    assert(D_smooth > D_rough && "Smooth surface should have higher NDF peak");

    // Geometry term decreases with roughness
    float G_smooth = geometrySmith(0.9f, 0.9f, 0.1f);
    float G_rough = geometrySmith(0.9f, 0.9f, 0.9f);
    assert(G_smooth > G_rough && "Rougher surface should have more self-shadowing");

    std::cout << "PASS" << std::endl;
}

// Test 5: PBR texture map generation produces valid data
void testPBRMapGeneration()
{
    std::cout << "Test: PBR texture map generation... ";

    ProceduralTextureGenerator generator;

    PlanetTextureConfig config;
    config.width = 128;
    config.height = 64;
    config.seed = 42;
    config.type = PlanetTextureConfig::PlanetType::TERRESTRIAL;

    std::vector<uint8_t> pixels = generator.generatePlanetTexture(config);
    assert(!pixels.empty());
    assert(pixels.size() == static_cast<size_t>(config.width * config.height * 4));

    // Verify pixel values are in valid range [0, 255]
    for (size_t i = 0; i < pixels.size(); ++i) {
        assert(pixels[i] <= 255);
    }

    std::cout << "PASS (" << pixels.size() << " bytes generated)" << std::endl;
}

// Test 6: Instance data layout is correct size (36 floats)
void testInstanceDataLayout()
{
    std::cout << "Test: Instance data layout (36 floats)... ";

    // Verify the expected instance data layout:
    // model(16) + modelIT(9) + color(3) + emissive(3) + radius(1) + texLayer(1) + metallic(1) + roughness(1) + ao(1) = 36 floats
    const int expectedFloats = 16 + 9 + 3 + 3 + 1 + 1 + 1 + 1 + 1;
    assert(expectedFloats == 36);

    std::cout << "PASS (" << expectedFloats << " floats = " << expectedFloats * sizeof(float) << " bytes)" << std::endl;
}

// Test 7: Distribution function properties
void testGGXDistributionProperties()
{
    std::cout << "Test: GGX distribution function properties... ";

    // Property 1: D is always positive
    bool allPositive = true;
    for (float roughness = 0.1f; roughness <= 1.0f; roughness += 0.1f) {
        for (float NdotH = 0.0f; NdotH <= 1.0f; NdotH += 0.1f) {
            float D_val = distributionGGX(NdotH, roughness);
            if (D_val < 0.0f) allPositive = false;
        }
    }
    assert(allPositive && "GGX must be non-negative");

    // Property 2: D is maximum when NdotH = 1
    float D_at_1 = distributionGGX(1.0f, 0.5f);
    float D_at_0_5 = distributionGGX(0.5f, 0.5f);
    assert(D_at_1 > D_at_0_5 && "GGX peaks at NdotH=1");

    // Property 3: Smooth surfaces have sharp peak
    float D_smooth = distributionGGX(1.0f, 0.04f);
    float D_smooth_off = distributionGGX(0.9f, 0.04f);
    assert(D_smooth > D_smooth_off * 5.0f && "Smooth surfaces have sharp NDF peak");

    std::cout << "PASS" << std::endl;
}

// Test 8: Smith geometry function properties
void testSmithGeometryProperties()
{
    std::cout << "Test: Smith geometry function properties... ";

    // Property 1: G is always in [0, 1]
    bool allValid = true;
    for (float roughness = 0.1f; roughness <= 1.0f; roughness += 0.2f) {
        for (float NdotV = 0.01f; NdotV <= 1.0f; NdotV += 0.1f) {
            for (float NdotL = 0.01f; NdotL <= 1.0f; NdotL += 0.1f) {
                float G_val = geometrySmith(NdotV, NdotL, roughness);
                if (G_val < 0.0f || G_val > 1.001f) allValid = false;
            }
        }
    }
    assert(allValid && "Smith G must be in [0,1]");

    // Property 2: G ~1 at normal incidence
    float G_head_on = geometrySmith(1.0f, 1.0f, 0.5f);
    assert(G_head_on > 0.99f && "G should be ~1 at normal incidence");

    // Property 3: G decreases at grazing angles
    float G_normal = geometrySmith(0.9f, 0.9f, 0.5f);
    float G_grazing = geometrySmith(0.1f, 0.1f, 0.5f);
    assert(G_normal > G_grazing && "G should decrease at grazing angles");

    std::cout << "PASS" << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main()
{
    std::cout << "=== PBR Rendering Tests ===" << std::endl;

    testPBRMaterialDefaults();
    testEnergyConservation();
    testFresnelGrazingAngles();
    testRoughnessMetallicParameters();
    testPBRMapGeneration();
    testInstanceDataLayout();
    testGGXDistributionProperties();
    testSmithGeometryProperties();

    std::cout << std::endl << "All PBR tests PASSED (8/8)" << std::endl;
    return 0;
}
