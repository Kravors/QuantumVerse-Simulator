/**
 * @file ComputeShader.h
 * @brief OpenGL compute shader management for GPU-accelerated N-body simulation
 *
 * Manages GLSL compute shader programs with SSBO binding for parallel
 * gravitational force calculation. Requires OpenGL 4.3+ (compute shader support).
 */

#ifndef QUANTUMVERSE_COMPUTE_SHADER_H
#define QUANTUMVERSE_COMPUTE_SHADER_H

#include <string>
#include <cstdint>

namespace quantumverse {

class ComputeShader {
public:
    ComputeShader();
    ~ComputeShader();

    ComputeShader(const ComputeShader&) = delete;
    ComputeShader& operator=(const ComputeShader&) = delete;

    bool loadFromFile(const std::string& filepath);
    bool loadFromSource(const std::string& source);
    void destroy();

    void dispatch(uint32_t numGroupsX, uint32_t numGroupsY = 1, uint32_t numGroupsZ = 1) const;
    void use() const;

    void setUniform(const char* name, float value) const;
    void setUniform(const char* name, int value) const;
    void setUniform(const char* name, uint32_t value) const;

    bool isLoaded() const { return m_programId != 0; }
    uint32_t programId() const { return m_programId; }

    static bool isComputeSupported();

private:
    bool compileComputeShader(const std::string& source);
    bool getShaderStage(uint32_t& shaderId, uint32_t type, const char* source);

    uint32_t m_programId;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_COMPUTE_SHADER_H
