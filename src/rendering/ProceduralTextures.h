/**
 * @file ProceduralTextures.h
 * @brief High-resolution procedural texture generation for celestial bodies
 *
 * Implements procedural texture generation using Perlin noise and other
 * algorithms to create realistic planet surfaces without external assets.
 * Supports real-time customization via UI parameters.
 */

#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include <random>

namespace quantumverse {

/**
 * @brief Configuration for procedural planet texture generation
 */
struct PlanetTextureConfig {
    // Base color properties
    float baseColor[3] = {0.5f, 0.5f, 0.5f};  // Base RGB color
    float colorVariation = 0.3f;              // Color variation amount
    
    // Noise parameters
    float noiseScale = 1.0f;                    // Scale of noise pattern
    float noiseOctaves = 4;                     // Number of noise octaves
    float noisePersistence = 0.5f;              // Noise persistence
    float noiseLacunarity = 2.0f;               // Noise lacunarity
    
    // Surface features
    float continentScale = 0.5f;                // Continent size scale
    float mountainHeight = 0.3f;                // Mountain height variation
    float oceanDepth = 0.2f;                    // Ocean depth variation
    
    // Atmospheric effects
    float atmosphereStrength = 0.0f;            // Atmosphere glow strength
    float cloudCoverage = 0.0f;                 // Cloud coverage (0-1)
    float cloudSharpness = 0.5f;                // Cloud edge sharpness
    
    // Texture resolution
    int width = 1024;
    int height = 512;
    
    // Planet type presets
    enum class PlanetType {
        TERRESTRIAL,    // Rocky planet with continents/oceans
        GAS_GIANT,      // Gas giant with bands
        ICE_WORLD,      // Ice world with cryovolcanism
        BARREN,         // Barren rocky world
        LAVA_WORLD,     // Volcanic world
        CUSTOM          // Custom parameters
    };
    PlanetType type = PlanetType::TERRESTRIAL;
    
    // Random seed for variation
    uint32_t seed = 42;
};

/**
 * @brief High-resolution procedural texture generator for celestial bodies
 */
class ProceduralTextureGenerator {
public:
    ProceduralTextureGenerator();
    ~ProceduralTextureGenerator() = default;
    
    /**
     * @brief Generate a procedural texture for a planet
     * @param config Texture configuration parameters
     * @return Vector of RGBA pixels (width * height * 4 elements)
     */
    std::vector<uint8_t> generatePlanetTexture(const PlanetTextureConfig& config);
    
    /**
     * @brief Generate a star surface texture
     * @param config Configuration (color, noise parameters)
     * @return Vector of RGBA pixels
     */
    std::vector<uint8_t> generateStarTexture(const PlanetTextureConfig& config);
    
    /**
     * @brief Generate a gas giant band texture
     * @param config Configuration with color bands
     * @return Vector of RGBA pixels
     */
    std::vector<uint8_t> generateGasGiantTexture(const PlanetTextureConfig& config);
    
    /**
     * @brief Generate an ice world texture
     * @param config Configuration
     * @return Vector of RGBA pixels
     */
    std::vector<uint8_t> generateIceWorldTexture(const PlanetTextureConfig& config);
    
    /**
     * @brief Generate a barren rocky texture
     * @param config Configuration
     * @return Vector of RGBA pixels
     */
    std::vector<uint8_t> generateBarrenTexture(const PlanetTextureConfig& config);
    
    /**
     * @brief Generate a lava world texture
     * @param config Configuration
     * @return Vector of RGBA pixels
     */
    std::vector<uint8_t> generateLavaWorldTexture(const PlanetTextureConfig& config);

private:
    // Noise generation
    float generatePerlinNoise(float x, float y, uint32_t seed) const;
    float generateFractalNoise(float x, float y, const PlanetTextureConfig& config) const;
    float generateFractalNoise(float x, float y, uint32_t seed) const;
    
    // Color mapping
    void mapHeightToColor(float height, float y, const PlanetTextureConfig& config,
                         float& r, float& g, float& b) const;
    
    // Feature generation
    float generateContinents(float x, float y, const PlanetTextureConfig& config) const;
    float generateMountains(float x, float y, const PlanetTextureConfig& config) const;
    float generateClouds(float x, float y, const PlanetTextureConfig& config) const;
    
    // Utility
    float smoothstep(float edge0, float edge1, float x) const;
    float clamp(float x, float minVal, float maxVal) const;
    
    // Permutation table for Perlin noise
    mutable std::vector<int> m_permutation;
    mutable bool m_noiseInitialized = false;
    void initializeNoise();
};

} // namespace quantumverse