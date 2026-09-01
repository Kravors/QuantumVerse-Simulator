/**
 * @file ComputeShader.cpp
 * @brief OpenGL compute shader implementation for GPU-accelerated N-body simulation
 *
 * Provides compute shader compilation, linking, and dispatch for SSBO-based
 * gravitational force calculation. Requires OpenGL 4.3+.
 */

#include "glad.h"
#include "ComputeShader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>

namespace quantumverse {

ComputeShader::ComputeShader() : m_programId(0) {}

ComputeShader::~ComputeShader() {
    destroy();
}

void ComputeShader::destroy() {
    if (m_programId != 0) {
        glDeleteProgram(m_programId);
        m_programId = 0;
    }
}

bool ComputeShader::isComputeSupported() {
    GLint major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    return (major > 4) || (major == 4 && minor >= 3);
}

bool ComputeShader::getShaderStage(uint32_t& shaderId, uint32_t type, const char* source) {
    shaderId = glCreateShader(type);
    glShaderSource(shaderId, 1, &source, nullptr);
    glCompileShader(shaderId);

    GLint success = 0;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[2048];
        glGetShaderInfoLog(shaderId, sizeof(infoLog), nullptr, infoLog);
        std::fprintf(stderr, "ERROR: Compute shader compilation failed:\n%s\n", infoLog);
        std::fprintf(stderr, "Shader source:\n%s\n", source);
        glDeleteShader(shaderId);
        shaderId = 0;
        return false;
    }
    return true;
}

bool ComputeShader::compileComputeShader(const std::string& source) {
    destroy();

    uint32_t computeShader = 0;
    if (!getShaderStage(computeShader, GL_COMPUTE_SHADER, source.c_str())) {
        return false;
    }

    m_programId = glCreateProgram();
    glAttachShader(m_programId, computeShader);
    glLinkProgram(m_programId);

    GLint success = 0;
    glGetProgramiv(m_programId, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[2048];
        glGetProgramInfoLog(m_programId, sizeof(infoLog), nullptr, infoLog);
        std::fprintf(stderr, "ERROR: Compute program linking failed:\n%s\n", infoLog);
        glDeleteProgram(m_programId);
        m_programId = 0;
        glDeleteShader(computeShader);
        return false;
    }

    glDeleteShader(computeShader);
    std::fprintf(stderr, "[ComputeShader] Program compiled successfully (id=%u)\n", m_programId);
    return true;
}

bool ComputeShader::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::fprintf(stderr, "ERROR: Cannot open compute shader file: %s\n", filepath.c_str());
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return compileComputeShader(buffer.str());
}

bool ComputeShader::loadFromSource(const std::string& source) {
    return compileComputeShader(source);
}

void ComputeShader::use() const {
    if (m_programId != 0) {
        glUseProgram(m_programId);
    }
}

void ComputeShader::dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ) const {
    if (m_programId != 0) {
        glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
    }
}

void ComputeShader::setUniform(const char* name, float value) const {
    if (m_programId == 0) return;
    GLint loc = glGetUniformLocation(static_cast<GLuint>(m_programId), name);
    if (loc >= 0) glUniform1f(loc, value);
}

void ComputeShader::setUniform(const char* name, int value) const {
    if (m_programId == 0) return;
    GLint loc = glGetUniformLocation(static_cast<GLuint>(m_programId), name);
    if (loc >= 0) glUniform1i(loc, value);
}

void ComputeShader::setUniform(const char* name, uint32_t value) const {
    if (m_programId == 0) return;
    GLint loc = glGetUniformLocation(static_cast<GLuint>(m_programId), name);
    if (loc >= 0) glUniform1ui(loc, value);
}

} // namespace quantumverse
