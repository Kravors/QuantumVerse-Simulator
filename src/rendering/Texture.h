/**
 * @file Texture.h
 * @brief OpenGL texture loading utility using stb_image
 *
 * Provides a simple RAII wrapper around OpenGL 2D textures loaded
 * from image files via stb_image.h. Supports GL_TEXTURE_2D_ARRAY
 * for multi-layer celestial body textures.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <unordered_map>
#include <functional>

namespace quantumverse {

/**
 * @brief RAII wrapper for an OpenGL 2D texture
 */
class Texture
{
public:
    /**
     * @brief Desired number of color channels when loading
     */
    enum class Channels {
        Auto = 0,   ///< Original number of channels in file
        Grey = 1,   ///< Force 1-channel (grayscale)
        GreyAlpha = 2, ///< Force 2-channel (grayscale + alpha)
        Rgb = 3,    ///< Force 3-channel (RGB)
        Rgba = 4    ///< Force 4-channel (RGBA)
    };

    /**
     * @brief Default constructor - creates an empty texture object
     */
    Texture();

    /**
     * @brief Destructor - releases OpenGL texture resource
     */
    ~Texture();

    // Non-copyable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Movable
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    /**
     * @brief Load a texture from a file path
     * @param filepath Path to the image file (PNG, JPEG, BMP, etc.)
     * @param channels Desired channel count (default: auto-detect)
     * @param generateMipmaps Whether to generate mipmaps (default: true)
     * @return true if the texture was loaded successfully
     *
     * If the file cannot be opened or decoded, a 1x1 magenta fallback
     * texture is created so that missing textures are visually obvious.
     */
    bool loadFromFile(const std::string& filepath,
                      Channels channels = Channels::Auto,
                      bool generateMipmaps = true);

    /**
     * @brief Load a texture from raw pixel data in memory
     * @param data Pointer to raw pixel bytes
     * @param width Image width in pixels
     * @param height Image height in pixels
     * @param channels Number of channels (1-4)
     * @param generateMipmaps Whether to generate mipmaps
     * @return true on success
     */
    bool loadFromMemory(const unsigned char* data,
                        int width, int height, int channels,
                        bool generateMipmaps = true);

    /**
     * @brief Bind this texture to a texture unit
     * @param unit Texture unit index (e.g., 0 for GL_TEXTURE0)
     */
    void bind(unsigned int unit = 0) const;

    /**
     * @brief Unbind the texture from a texture unit
     * @param unit Texture unit index
     */
    static void unbind(unsigned int unit = 0);

    /**
     * @brief Get the OpenGL texture ID
     */
    unsigned int getId() const noexcept { return m_id; }

    /**
     * @brief Get the texture width in pixels
     */
    int getWidth() const noexcept { return m_width; }

    /**
     * @brief Get the texture height in pixels
     */
    int getHeight() const noexcept { return m_height; }

    /**
     * @brief Get the number of channels
     */
    int getChannels() const noexcept { return m_channels; }

    /**
     * @brief Check if the texture is valid (loaded)
     */
    bool isValid() const noexcept { return m_id != 0; }

    /**
     * @brief Get the OpenGL internal format
     * @param channels Number of channels (1-4)
     * @return Corresponding GL_RED, GL_RG, GL_RGB, or GL_RGBA
     */
    static unsigned int getGLFormat(int channels);

private:
    unsigned int m_id;       ///< OpenGL texture name
    int m_width;             ///< Texture width in pixels
    int m_height;            ///< Texture height in pixels
    int m_channels;          ///< Number of color channels

    /**
     * @brief Create a 1x1 magenta fallback texture
     */
    void createFallbackTexture();

    /**
     * @brief Convert Channels enum to OpenGL format
     */
    static unsigned int channelsToGLFormat(int channels);

    /**
     * @brief Convert Channels enum to OpenGL type
     */
    static unsigned int channelsToGLType(int channels);
};

/**
 * @brief Manager for a GL_TEXTURE_2D_ARRAY of celestial body textures
 *
 * Loads multiple planet textures into a single texture array,
 * allowing per-instance layer indexing in the vertex shader.
 * All textures in the array must have the same dimensions.
 */
class TextureArray
{
public:
    TextureArray();
    ~TextureArray();

    // Non-copyable
    TextureArray(const TextureArray&) = delete;
    TextureArray& operator=(const TextureArray&) = delete;

    // Movable
    TextureArray(TextureArray&& other) noexcept;
    TextureArray& operator=(TextureArray&& other) noexcept;

    /**
     * @brief Initialize the texture array with a maximum number of layers
     * @param maxLayers Maximum number of texture layers
     * @param width Width of each texture layer (all must be the same)
     * @param height Height of each texture layer
     * @return true on success
     */
    bool initialize(int maxLayers, int width = 1024, int height = 512);

    /**
     * @brief Add a texture layer from a file
     * @param filepath Path to the image file
     * @return Layer index, or -1 on failure
     */
    int addLayerFromFile(const std::string& filepath);

    /**
     * @brief Add a texture layer from raw pixel data
     * @param data Raw pixel bytes
     * @param width Layer width
     * @param height Layer height
     * @param channels Number of channels (1-4)
     * @return Layer index, or -1 on failure
     */
    int addLayerFromMemory(const unsigned char* data,
                           int width, int height, int channels);

    /**
     * @brief Bind the texture array to a texture unit
     * @param unit Texture unit index
     */
    void bind(unsigned int unit = 0) const;

    /**
     * @brief Unbind the texture array
     * @param unit Texture unit index
     */
    static void unbind(unsigned int unit = 0);

    unsigned int getId() const noexcept { return m_id; }
    int getWidth() const noexcept { return m_width; }
    int getHeight() const noexcept { return m_height; }
    int getLayerCount() const noexcept { return m_layerCount; }
    int getMaxLayers() const noexcept { return m_maxLayers; }
    bool isValid() const noexcept { return m_id != 0; }

private:
    unsigned int m_id;
    int m_width;
    int m_height;
    int m_maxLayers;
    int m_layerCount;
    int m_channels;
};

/**
 * @brief Texture cache for performance optimization
 *
 * Caches generated textures to avoid redundant generation.
 * Uses LRU eviction policy for memory management.
 */
class TextureCache {
public:
    TextureCache();
    ~TextureCache();

    // Non-copyable
    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;

    /**
     * @brief Get a cached texture or generate a new one
     * @param key Cache key (e.g., planet type + seed)
     * @param generator Function to generate texture if not cached
     * @return Texture data
     */
    std::vector<uint8_t> getOrGenerate(
        const std::string& key,
        std::function<std::vector<uint8_t>()> generator
    );

    /**
     * @brief Clear the cache
     */
    void clear();

    /**
     * @brief Get cache size
     */
    size_t size() const { return m_cache.size(); }

    /**
     * @brief Set maximum cache size (in bytes)
     */
    void setMaxSize(size_t bytes) { m_maxSizeBytes = bytes; }

private:
    struct CachedTexture {
        std::vector<uint8_t> data;
        size_t lastAccess;
        size_t size;
    };

    std::unordered_map<std::string, CachedTexture> m_cache;
    size_t m_maxSizeBytes;
    size_t m_currentSizeBytes;
    size_t m_accessCounter;

    void evictLRU();
};

} // namespace quantumverse