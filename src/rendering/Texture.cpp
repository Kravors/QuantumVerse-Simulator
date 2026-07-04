/**
 * @file Texture.cpp
 * @brief Implementation of OpenGL texture loading utility
 */

#include "Texture.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// glad.h MUST be first after Windows headers to provide OpenGL function prototypes
#include "../../third_party/glad/glad.h"

// stb_image for loading image files
#define STB_IMAGE_IMPLEMENTATION
#include "../../third_party/stb_image.h"

#include <cstring>
#include <iostream>
#include <algorithm>

namespace quantumverse {

// ============================================================================
// Texture
// ============================================================================

Texture::Texture()
    : m_id(0)
    , m_width(0)
    , m_height(0)
    , m_channels(0)
{
}

Texture::~Texture()
{
    if (m_id) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
}

Texture::Texture(Texture&& other) noexcept
    : m_id(other.m_id)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_channels(other.m_channels)
{
    other.m_id = 0;
    other.m_width = 0;
    other.m_height = 0;
    other.m_channels = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other) {
        if (m_id) glDeleteTextures(1, &m_id);
        m_id = other.m_id;
        m_width = other.m_width;
        m_height = other.m_height;
        m_channels = other.m_channels;
        other.m_id = 0;
        other.m_width = 0;
        other.m_height = 0;
        other.m_channels = 0;
    }
    return *this;
}

bool Texture::loadFromFile(const std::string& filepath,
                            Channels channels,
                            bool generateMipmaps)
{
    int reqComp = static_cast<int>(channels);
    int w = 0, h = 0, comp = 0;

    unsigned char* data = stbi_load(filepath.c_str(), &w, &h, &comp, reqComp);

    if (!data) {
        std::cerr << "Texture: Failed to load '" << filepath << "'" << std::endl;
        createFallbackTexture();
        return false;
    }

    // If channels was Auto, stbi_load returns the original count
    int actualChannels = (reqComp == 0) ? comp : reqComp;

    bool success = loadFromMemory(data, w, h, actualChannels, generateMipmaps);
    stbi_image_free(data);
    return success;
}

bool Texture::loadFromMemory(const unsigned char* data,
                              int width, int height, int channels,
                              bool generateMipmaps)
{
    if (!data || width <= 0 || height <= 0 || channels < 1 || channels > 4) {
        std::cerr << "Texture: Invalid image data (" << width << "x" << height
                  << ", " << channels << " channels)" << std::endl;
        createFallbackTexture();
        return false;
    }

    // Delete old texture if it exists
    if (m_id) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }

    m_width = width;
    m_height = height;
    m_channels = channels;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixel data
    unsigned int format = channelsToGLFormat(channels);
    unsigned int type = channelsToGLType(channels);

    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format),
                 width, height, 0, format, type, data);

    if (generateMipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

void Texture::bind(unsigned int unit) const
{
    if (m_id) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, m_id);
    }
}

void Texture::unbind(unsigned int unit)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, 0);
}

unsigned int Texture::getGLFormat(int channels)
{
    switch (channels) {
        case 1: return GL_RED;
        case 2: return GL_RG;
        case 3: return GL_RGB;
        case 4: return GL_RGBA;
        default: return GL_RGB;
    }
}

void Texture::createFallbackTexture()
{
    // 1x1 magenta pixel - clearly indicates a missing texture
    unsigned char fallback[4] = {255, 0, 255, 255};
    loadFromMemory(fallback, 1, 1, 4, false);
}

unsigned int Texture::channelsToGLFormat(int channels)
{
    switch (channels) {
        case 1: return GL_RED;
        case 2: return GL_RG;
        case 3: return GL_RGB;
        case 4: return GL_RGBA;
        default: return GL_RGB;
    }
}

unsigned int Texture::channelsToGLType(int channels)
{
    return GL_UNSIGNED_BYTE;
}

// ============================================================================
// TextureArray
// ============================================================================

TextureArray::TextureArray()
    : m_id(0)
    , m_width(0)
    , m_height(0)
    , m_maxLayers(0)
    , m_layerCount(0)
    , m_channels(0)
{
}

TextureArray::~TextureArray()
{
    if (m_id) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
}

TextureArray::TextureArray(TextureArray&& other) noexcept
    : m_id(other.m_id)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_maxLayers(other.m_maxLayers)
    , m_layerCount(other.m_layerCount)
    , m_channels(other.m_channels)
{
    other.m_id = 0;
    other.m_maxLayers = 0;
    other.m_layerCount = 0;
}

TextureArray& TextureArray::operator=(TextureArray&& other) noexcept
{
    if (this != &other) {
        if (m_id) glDeleteTextures(1, &m_id);
        m_id = other.m_id;
        m_width = other.m_width;
        m_height = other.m_height;
        m_maxLayers = other.m_maxLayers;
        m_layerCount = other.m_layerCount;
        m_channels = other.m_channels;
        other.m_id = 0;
        other.m_maxLayers = 0;
        other.m_layerCount = 0;
    }
    return *this;
}

bool TextureArray::initialize(int maxLayers, int width, int height)
{
    if (maxLayers <= 0 || width <= 0 || height <= 0) {
        std::cerr << "TextureArray: Invalid parameters (layers="
                  << maxLayers << ", " << width << "x" << height << ")" << std::endl;
        return false;
    }

    if (m_id) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }

    m_width = width;
    m_height = height;
    m_maxLayers = maxLayers;
    m_layerCount = 0;
    m_channels = 4; // Default to RGBA

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_id);

    // Allocate storage for the array (layers are added lazily)
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8,
                   width, height, maxLayers);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    return true;
}

int TextureArray::addLayerFromFile(const std::string& filepath)
{
    if (m_layerCount >= m_maxLayers) {
        std::cerr << "TextureArray: Maximum layer count (" << m_maxLayers
                  << ") reached" << std::endl;
        return -1;
    }

    int w = 0, h = 0, comp = 0;
    unsigned char* data = stbi_load(filepath.c_str(), &w, &h, &comp, 4);

    if (!data) {
        std::cerr << "TextureArray: Failed to load layer '" << filepath << "'" << std::endl;
        return -1;
    }

    // Resize to match array dimensions if needed
    if (w != m_width || h != m_height) {
        std::cerr << "TextureArray: Layer size mismatch (" << w << "x" << h
                  << " vs array " << m_width << "x" << m_height << ")" << std::endl;
        stbi_image_free(data);
        return -1;
    }

    int layerIndex = m_layerCount++;
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_id);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                    0, 0, layerIndex,
                    m_width, m_height, 1,
                    GL_RGBA, GL_UNSIGNED_BYTE, data);

    // Regenerate mipmaps after adding a new layer so all levels are consistent
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    stbi_image_free(data);
    return layerIndex;
}

int TextureArray::addLayerFromMemory(const unsigned char* data,
                                      int width, int height, int channels)
{
    if (m_layerCount >= m_maxLayers) {
        std::cerr << "TextureArray: Maximum layer count (" << m_maxLayers
                  << ") reached" << std::endl;
        return -1;
    }

    if (!data || width != m_width || height != m_height) {
        std::cerr << "TextureArray: Invalid layer data or size mismatch" << std::endl;
        return -1;
    }

    // Convert to RGBA if needed
    std::vector<unsigned char> rgbaData;
    const unsigned char* srcData = data;

    if (channels != 4) {
        size_t pixelCount = static_cast<size_t>(width) * height;
        rgbaData.resize(pixelCount * 4);
        for (size_t i = 0; i < pixelCount; ++i) {
            rgbaData[i * 4 + 0] = data[i * channels + 0]; // R
            rgbaData[i * 4 + 1] = (channels >= 2) ? data[i * channels + 1] : data[i * channels + 0]; // G or R
            rgbaData[i * 4 + 2] = (channels >= 3) ? data[i * channels + 2] : data[i * channels + 0]; // B or R
            rgbaData[i * 4 + 3] = (channels >= 4) ? data[i * channels + 3] : 255; // A
        }
        srcData = rgbaData.data();
    }

    int layerIndex = m_layerCount++;
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_id);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
                    0, 0, layerIndex,
                    m_width, m_height, 1,
                    GL_RGBA, GL_UNSIGNED_BYTE, srcData);

    // Regenerate mipmaps after adding a new layer so all levels are consistent
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    return layerIndex;
}

void TextureArray::bind(unsigned int unit) const
{
    if (m_id) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_id);
    }
}

void TextureArray::unbind(unsigned int unit)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

// ============================================================================
// TextureCache
// ============================================================================

TextureCache::TextureCache()
    : m_maxSizeBytes(512 * 1024 * 1024) // 512 MB default
    , m_currentSizeBytes(0)
    , m_accessCounter(0)
{
}

TextureCache::~TextureCache()
{
    clear();
}

std::vector<uint8_t> TextureCache::getOrGenerate(
    const std::string& key,
    std::function<std::vector<uint8_t>()> generator)
{
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        // Cache hit - update access time
        it->second.lastAccess = ++m_accessCounter;
        return it->second.data;
    }

    // Cache miss - generate texture
    auto data = generator();
    
    // Add to cache
    CachedTexture cached;
    cached.data = std::move(data);
    cached.lastAccess = ++m_accessCounter;
    cached.size = cached.data.size();
    
    m_cache[key] = std::move(cached);
    m_currentSizeBytes += m_cache[key].size;
    
    // Evict if over limit
    if (m_currentSizeBytes > m_maxSizeBytes) {
        evictLRU();
    }
    
    return m_cache[key].data;
}

void TextureCache::clear()
{
    m_cache.clear();
    m_currentSizeBytes = 0;
    m_accessCounter = 0;
}

void TextureCache::evictLRU()
{
    if (m_cache.empty()) return;
    
    // Find least recently used entry
    auto lruIt = m_cache.begin();
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it->second.lastAccess < lruIt->second.lastAccess) {
            lruIt = it;
        }
    }
    
    m_currentSizeBytes -= lruIt->second.size;
    m_cache.erase(lruIt);
}

} // namespace quantumverse