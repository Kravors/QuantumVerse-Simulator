/**
 * @file ProceduralTextures.cpp
 * @brief Implementation of procedural texture generation
 */

#include "ProceduralTextures.h"
#include <algorithm>
#include <random>

namespace quantumverse {

ProceduralTextureGenerator::ProceduralTextureGenerator() {
    initializeNoise();
}

void ProceduralTextureGenerator::initializeNoise() {
    if (m_noiseInitialized) return;
    
    // Create permutation table for Perlin noise
    m_permutation.resize(256);
    for (int i = 0; i < 256; i++) {
        m_permutation[i] = i;
    }
    
    // Shuffle using Fisher-Yates
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(m_permutation.begin(), m_permutation.end(), g);
    
    // Duplicate for overflow
    m_permutation.insert(m_permutation.end(), m_permutation.begin(), m_permutation.end());
    m_noiseInitialized = true;
}

float ProceduralTextureGenerator::generatePerlinNoise(float x, float y, uint32_t seed) const {
    // Hash seed into permutation offset
    int offset = seed % 256;
    
    int X = (int)floor(x) & 255;
    int Y = (int)floor(y) & 255;
    
    float u = x - floor(x);
    float v = y - floor(y);
    
    // Fade curves
    float uFade = u * u * u * (u * (u * 6 - 15) + 10);
    float vFade = v * v * v * (v * (v * 6 - 15) + 10);
    
    // Hash coordinates
    int A = m_permutation[(X + offset) & 255] + Y;
    int B = m_permutation[(X + 1 + offset) & 255] + Y;
    
    // Gradient lookup (simple 2D gradient)
    float gradA = m_permutation[A & 255] * 0.01f;
    float gradB = m_permutation[B & 255] * 0.01f;
    
    // Interpolate
    float result = (1 - uFade) * gradA + uFade * gradB;
    return (2 * result);
}

float ProceduralTextureGenerator::generateFractalNoise(float x, float y, 
                                                      const PlanetTextureConfig& config) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = config.noiseScale;
    
    for (int i = 0; i < (int)config.noiseOctaves; i++) {
        value += amplitude * generatePerlinNoise(x * frequency, y * frequency, config.seed + i);
        amplitude *= config.noisePersistence;
        frequency *= config.noiseLacunarity;
    }
    
    return value;
}

float ProceduralTextureGenerator::generateFractalNoise(float x, float y, uint32_t seed) const {
    // Create a temporary config with default values
    PlanetTextureConfig tempConfig;
    tempConfig.seed = seed;
    return generateFractalNoise(x, y, tempConfig);
}

float ProceduralTextureGenerator::smoothstep(float edge0, float edge1, float x) const {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3 - 2 * t);
}

float ProceduralTextureGenerator::clamp(float x, float minVal, float maxVal) const {
    return std::max(minVal, std::min(maxVal, x));
}

void ProceduralTextureGenerator::mapHeightToColor(float height, float y, 
                                                   const PlanetTextureConfig& config,
                                                   float& r, float& g, float& b) const {
    // Map height to color based on planet type
    switch (config.type) {
        case PlanetTextureConfig::PlanetType::TERRESTRIAL: {
            // Ocean depths (dark blue)
            if (height < 0.3f) {
                float t = height / 0.3f;
                r = 0.0f + t * 0.1f;
                g = 0.1f + t * 0.2f;
                b = 0.3f + t * 0.3f;
            }
            // Land masses (green/brown)
            else if (height < 0.7f) {
                float t = (height - 0.3f) / 0.4f;
                r = 0.2f + t * 0.3f;
                g = 0.4f + t * 0.2f;
                b = 0.1f + t * 0.1f;
            }
            // Mountains (white/gray)
            else {
                float t = (height - 0.7f) / 0.3f;
                r = 0.5f + t * 0.5f;
                g = 0.5f + t * 0.3f;
                b = 0.5f + t * 0.2f;
            }
            break;
        }
        case PlanetTextureConfig::PlanetType::GAS_GIANT: {
            // Banded colors
            float band = sin(y * 10.0f + config.seed * 0.1f);
            r = 0.5f + band * 0.3f;
            g = 0.3f + band * 0.2f;
            b = 0.7f - band * 0.2f;
            break;
        }
        case PlanetTextureConfig::PlanetType::ICE_WORLD: {
            // Ice blues and whites
            r = 0.7f + height * 0.3f;
            g = 0.8f + height * 0.2f;
            b = 0.9f + height * 0.1f;
            break;
        }
        case PlanetTextureConfig::PlanetType::BARREN: {
            // Rocky browns and reds
            r = 0.4f + height * 0.2f;
            g = 0.3f + height * 0.1f;
            b = 0.2f + height * 0.1f;
            break;
        }
        case PlanetTextureConfig::PlanetType::LAVA_WORLD: {
            // Lava reds and oranges
            r = 0.8f + height * 0.2f;
            g = 0.2f + height * 0.3f;
            b = 0.1f + height * 0.1f;
            break;
        }
        default:
            r = config.baseColor[0];
            g = config.baseColor[1];
            b = config.baseColor[2];
    }
}

float ProceduralTextureGenerator::generateContinents(float x, float y, 
                                                     const PlanetTextureConfig& config) const {
    float noise = generateFractalNoise(x, y, config);
    // Threshold to create land/ocean
    return smoothstep(0.3f, 0.7f, noise);
}

float ProceduralTextureGenerator::generateMountains(float x, float y,
                                                   const PlanetTextureConfig& config) const {
    PlanetTextureConfig tempConfig = config;
    tempConfig.seed = config.seed + 100;
    float noise = generateFractalNoise(x * 2.0f, y * 2.0f, tempConfig);
    return std::max(0.0f, noise) * config.mountainHeight;
}

float ProceduralTextureGenerator::generateClouds(float x, float y,
                                                const PlanetTextureConfig& config) const {
    if (config.cloudCoverage <= 0.0f) return 0.0f;
    
    PlanetTextureConfig tempConfig = config;
    tempConfig.seed = config.seed + 200;
    float noise = generateFractalNoise(x, y, tempConfig);
    float coverage = smoothstep(1.0f - config.cloudCoverage, 1.0f, noise);
    return pow(coverage, 1.0f / config.cloudSharpness);
}

std::vector<uint8_t> ProceduralTextureGenerator::generatePlanetTexture(
    const PlanetTextureConfig& config) {
    
    std::vector<uint8_t> pixels(config.width * config.height * 4);
    
    for (int y = 0; y < config.height; y++) {
        for (int x = 0; x < config.width; x++) {
            // Normalize coordinates
            float nx = (float)x / config.width;
            float ny = (float)y / config.height;
            
            // Generate height map
            float height = generateContinents(nx, ny, config);
            height += generateMountains(nx, ny, config);
            height = clamp(height, 0.0f, 1.0f);
            
            // Map to color
            float r, g, b;
            mapHeightToColor(height, ny, config, r, g, b);
            
            // Add clouds
            float clouds = generateClouds(nx, ny, config);
            r = r * (1.0f - clouds) + 1.0f * clouds;
            g = g * (1.0f - clouds) + 1.0f * clouds;
            b = b * (1.0f - clouds) + 1.0f * clouds;
            
            // Store pixel
            int idx = (y * config.width + x) * 4;
            pixels[idx + 0] = (uint8_t)(r * 255);
            pixels[idx + 1] = (uint8_t)(g * 255);
            pixels[idx + 2] = (uint8_t)(b * 255);
            pixels[idx + 3] = 255; // Alpha
        }
    }
    
    return pixels;
}

std::vector<uint8_t> ProceduralTextureGenerator::generateStarTexture(
    const PlanetTextureConfig& config) {
    
    std::vector<uint8_t> pixels(config.width * config.height * 4);
    
    for (int y = 0; y < config.height; y++) {
        for (int x = 0; x < config.width; x++) {
            float nx = (float)x / config.width - 0.5f;
            float ny = (float)y / config.height - 0.5f;
            
            // Distance from center
            float dist = sqrt(nx * nx + ny * ny);
            
            // Generate plasma-like noise
            float noise = generateFractalNoise(x * 0.1f, y * 0.1f, config.seed);
            
            // Star color based on noise
            float r = 1.0f;
            float g = 0.8f + noise * 0.2f;
            float b = 0.6f + noise * 0.4f;
            
            // Apply radial falloff
            float intensity = 1.0f - dist * 2.0f;
            intensity = std::max(0.0f, intensity);
            
            int idx = (y * config.width + x) * 4;
            pixels[idx + 0] = (uint8_t)(r * intensity * 255);
            pixels[idx + 1] = (uint8_t)(g * intensity * 255);
            pixels[idx + 2] = (uint8_t)(b * intensity * 255);
            pixels[idx + 3] = (uint8_t)(intensity * 255);
        }
    }
    
    return pixels;
}

std::vector<uint8_t> ProceduralTextureGenerator::generateGasGiantTexture(
    const PlanetTextureConfig& config) {
    
    std::vector<uint8_t> pixels(config.width * config.height * 4);
    
    for (int y = 0; y < config.height; y++) {
        for (int x = 0; x < config.width; x++) {
            float nx = (float)x / config.width;
            float ny = (float)y / config.height;
            
            // Create banded pattern
            float band = sin(ny * 10.0f + config.seed * 0.1f);
            PlanetTextureConfig tempConfig = config;
            tempConfig.seed = config.seed + 50;
            float turbulence = generateFractalNoise(nx, ny, tempConfig) * 0.3f;
            
            float r = 0.7f + (band + turbulence) * 0.15f;
            float g = 0.4f + (band + turbulence) * 0.1f;
            float b = 0.2f + (band + turbulence) * 0.2f;
            
            int idx = (y * config.width + x) * 4;
            pixels[idx + 0] = (uint8_t)(clamp(r, 0.0f, 1.0f) * 255);
            pixels[idx + 1] = (uint8_t)(clamp(g, 0.0f, 1.0f) * 255);
            pixels[idx + 2] = (uint8_t)(clamp(b, 0.0f, 1.0f) * 255);
            pixels[idx + 3] = 255;
        }
    }
    
    return pixels;
}

std::vector<uint8_t> ProceduralTextureGenerator::generateIceWorldTexture(
    const PlanetTextureConfig& config) {
    
    std::vector<uint8_t> pixels(config.width * config.height * 4);
    
    for (int y = 0; y < config.height; y++) {
        for (int x = 0; x < config.width; x++) {
            float nx = (float)x / config.width;
            float ny = (float)y / config.height;
            
            // Ice crack patterns
            float cracks = generateFractalNoise(nx * 5.0f, ny * 5.0f, config.seed);
            float cracksVis = smoothstep(0.7f, 0.9f, cracks);
            
            float r = 0.8f - cracksVis * 0.3f;
            float g = 0.9f - cracksVis * 0.2f;
            float b = 1.0f;
            
            int idx = (y * config.width + x) * 4;
            pixels[idx + 0] = (uint8_t)(r * 255);
            pixels[idx + 1] = (uint8_t)(g * 255);
            pixels[idx + 2] = (uint8_t)(b * 255);
            pixels[idx + 3] = 255;
        }
    }
    
    return pixels;
}

std::vector<uint8_t> ProceduralTextureGenerator::generateBarrenTexture(
    const PlanetTextureConfig& config) {
    
    std::vector<uint8_t> pixels(config.width * config.height * 4);
    
    for (int y = 0; y < config.height; y++) {
        for (int x = 0; x < config.width; x++) {
            float nx = (float)x / config.width;
            float ny = (float)y / config.height;
            
            // Rocky surface with craters
            float rocks = generateFractalNoise(nx * 10.0f, ny * 10.0f, config.seed);
            PlanetTextureConfig tempConfig = config;
            tempConfig.seed = config.seed + 100;
            float craters = generateFractalNoise(nx * 3.0f, ny * 3.0f, tempConfig);
            
            float r = 0.4f + rocks * 0.1f + craters * 0.1f;
            float g = 0.3f + rocks * 0.05f + craters * 0.05f;
            float b = 0.2f + rocks * 0.05f + craters * 0.05f;
            
            int idx = (y * config.width + x) * 4;
            pixels[idx + 0] = (uint8_t)(clamp(r, 0.0f, 1.0f) * 255);
            pixels[idx + 1] = (uint8_t)(clamp(g, 0.0f, 1.0f) * 255);
            pixels[idx + 2] = (uint8_t)(clamp(b, 0.0f, 1.0f) * 255);
            pixels[idx + 3] = 255;
        }
    }
    
    return pixels;
}

std::vector<uint8_t> ProceduralTextureGenerator::generateLavaWorldTexture(
    const PlanetTextureConfig& config) {
    
    std::vector<uint8_t> pixels(config.width * config.height * 4);
    
    for (int y = 0; y < config.height; y++) {
        for (int x = 0; x < config.width; x++) {
            float nx = (float)x / config.width;
            float ny = (float)y / config.height;
            
            // Lava flow patterns
            float flow = generateFractalNoise(nx * 5.0f, ny * 5.0f, config.seed);
            PlanetTextureConfig tempConfig = config;
            tempConfig.seed = config.seed + 200;
            float hotspots = generateFractalNoise(nx * 15.0f, ny * 15.0f, tempConfig);
            
            float r = 0.7f + flow * 0.2f + hotspots * 0.3f;
            float g = 0.2f + flow * 0.1f + hotspots * 0.2f;
            float b = 0.1f + flow * 0.05f;
            
            int idx = (y * config.width + x) * 4;
            pixels[idx + 0] = (uint8_t)(clamp(r, 0.0f, 1.0f) * 255);
            pixels[idx + 1] = (uint8_t)(clamp(g, 0.0f, 1.0f) * 255);
            pixels[idx + 2] = (uint8_t)(clamp(b, 0.0f, 1.0f) * 255);
            pixels[idx + 3] = 255;
        }
    }
    
    return pixels;
}

} // namespace quantumverse