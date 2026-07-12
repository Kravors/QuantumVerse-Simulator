/**
 * @file gl_check.h
 * @brief Conditional OpenGL error-checking macro
 *
 * Defines the `GL_CHECK()` macro. In Debug builds (or when
 * `QUANTUMVERSE_GL_DEBUG` is defined) it performs a synchronous
 * `glGetError()` query after each GL call, which is useful for catching
 * rendering bugs early. In Release builds it expands to a no-op so no
 * CPU-side sync stall is incurred on the hot render path.
 *
 * This is a compile-time guarantee: the macro body is fully replaced by
 * `((void)0)` when disabled, so no GL error query code is emitted.
 */

#pragma once

#include "../third_party/glad/glad.h"

#include <QDebug>

#if !defined(NDEBUG) || defined(QUANTUMVERSE_GL_DEBUG)
/**
 * @brief Query and log the OpenGL error state.
 *
 * Active in Debug builds and when `QUANTUMVERSE_GL_DEBUG` is set.
 * No-op (expands to `((void)0)`) in Release builds.
 */
#define GL_CHECK() do { \
    GLenum err = glad_glGetError(); \
    if (err != GL_NO_ERROR) { \
        qWarning() << "GL Error:" << err << " at " << __FILE__ << ":" << __LINE__; \
    } \
} while(0)
#else
/**
 * @brief No-op GL error check in Release builds.
 *
 * Compiled out entirely; emits no GL calls.
 */
#define GL_CHECK() ((void)0)
#endif
