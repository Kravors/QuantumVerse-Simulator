// Phase 5.7: Viewport State Leak Regression Test
//
// Guards the GL state machine against regressions where the FBO binding or
// polygon mode leaks across render passes.  Runs 10 frames of curvature grid
// + celestial body rendering, toggling wireframe on/off, and verifies that
// after every pass:
//   - A non-default FBO is still bound (not the default framebuffer 0)
//   - glPolygonMode is GL_FILL (wireframe does not leak)
//   - glGetError() is clean
//
// Run with: QT_QPA_PLATFORM=offscreen ./test_viewport_state_test

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "glad/glad.h"

#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QSurfaceFormat>

#include <cstring>
#include <cmath>
#include <iostream>
#include <vector>
#include <memory>
#include <string>

#include "rendering/CurvatureRenderer.h"
#include "rendering/CelestialBodyRenderer.h"
#include "rendering/ProceduralTextures.h"
#include "physics/CurvatureCalculator.h"
#include "spacetime/MetricTensor.h"
#include "physics/SingularityHandler.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int g_failures = 0;

static void check(bool cond, const char* msg)
{
    if (cond) {
        std::cout << "  [PASS] " << msg << std::endl;
    } else {
        std::cerr << "  [FAIL] " << msg << std::endl;
        ++g_failures;
    }
}

static void perspective(float* m, float fovy, float aspect, float n, float f)
{
    const float t = 1.0f / std::tan(fovy / 2.0f);
    std::memset(m, 0, 16 * sizeof(float));
    m[0] = t / aspect;
    m[5] = t;
    m[10] = (f + n) / (n - f);
    m[11] = -1.0f;
    m[14] = (2.0f * f * n) / (n - f);
}

static void translate(float* m, float x, float y, float z)
{
    std::memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
    m[12] = x; m[13] = y; m[14] = z;
}

/// @brief Verify that a valid (non-default) FBO is bound and no GL errors linger.
static bool checkState(const char* stage)
{
    bool pass = true;

    GLint fbo = -1;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    if (fbo == 0) {
        std::cerr << "  [FAIL] " << stage << ": No FBO bound (0)! Expected test FBO." << std::endl;
        pass = false;
    }

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cerr << "  [FAIL] " << stage << ": glGetError=0x" << std::hex << err
                  << std::dec << " (not clean)" << std::endl;
        pass = false;
    }

    return pass;
}

static int runMockGLMode();

int main(int argc, char* argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");

    QGuiApplication app(argc, argv);

    std::cout << "=== Viewport State Test (GL state leak detection) ===" << std::endl;

    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setVersion(4, 5);
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    fmt.setDepthBufferSize(24);

    QOffscreenSurface surface;
    surface.setFormat(fmt);
    surface.create();

    QOpenGLContext ctx;
    ctx.setFormat(fmt);
    if (!ctx.create()) {
        std::cerr << "  [SKIP] QOpenGLContext::create() failed — no usable GL context." << std::endl;
        return runMockGLMode();
    }
    ctx.makeCurrent(&surface);

    // ---- Load GLAD over this Qt context ----
    auto loader = [](const char* name) -> void* {
        QOpenGLContext* c = QOpenGLContext::currentContext();
        if (!c) return nullptr;
        return reinterpret_cast<void*>(c->getProcAddress(name));
    };
    if (!gladLoadGLLoader(loader)) {
        std::cerr << "  [SKIP] gladLoadGLLoader() failed — no usable GL context." << std::endl;
        return runMockGLMode();
    }
    check(true, "gladLoadGLLoader()");

    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    std::cout << "  GL_VERSION:  " << (version ? version : "?") << std::endl;
    std::cout << "  GL_RENDERER: " << (renderer ? renderer : "?") << std::endl;

    // ---- Offscreen test FBO (NOT the default framebuffer) ----
    const int FB_W = 512, FB_H = 512;
    uint8_t bg[4] = { 13, 13, 18, 255 };

    GLuint testFBO = 0, testColorTex = 0, testDepthRBO = 0;
    glGenFramebuffers(1, &testFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, testFBO);

    glGenTextures(1, &testColorTex);
    glBindTexture(GL_TEXTURE_2D, testColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FB_W, FB_H, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, testColorTex, 0);

    glGenRenderbuffers(1, &testDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, testDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, FB_W, FB_H);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, testDepthRBO);

    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "  [SKIP] Test FBO incomplete (0x" << std::hex << fboStatus
                  << "). Using mock mode." << std::dec << std::endl;
        return runMockGLMode();
    }

    // ---- Initialize CurvatureRenderer ----
    quantumverse::CurvatureRenderer curv(
        24, 100.0f, quantumverse::CurvatureMode::GRID_DEFORMATION);
    curv.initializeGL();
    check(curv.isInitialized(), "CurvatureRenderer.initializeGL()");

    auto metric = std::make_shared<quantumverse::MetricTensor>(
        quantumverse::MetricTensor::schwarzschild(1.0, 10.0,
            M_PI / 2.0, 0.0));
    curv.setMetric(metric);

    auto singularity = std::make_shared<quantumverse::SingularityHandler>(
        quantumverse::SingularityType::SCHWARZSCHILD,
        1.0e30, 0.0, 0.0,
        std::array<double, 3>{0.0, 0.0, 0.0});
    curv.addSingularity(singularity);

    // ---- Initialize CelestialBodyRenderer ----
    quantumverse::CelestialBodyRenderer bodyRenderer(
        quantumverse::CelestialBodyRenderer::QualityLevel::LOW, 32);
    bodyRenderer.initializeGL();
    check(bodyRenderer.isInitialized(), "CelestialBodyRenderer.initializeGL()");

    quantumverse::CelestialBodyInstance star;
    star.objectId = "sun";
    star.isStar = true;
    star.radius = 16.0f;
    star.position[0] = 25.0f;
    star.emissive[0] = 1.0f; star.emissive[1] = 0.85f; star.emissive[2] = 0.4f;
    star.color[0] = 1.0f; star.color[1] = 0.9f; star.color[2] = 0.6f;
    check(bodyRenderer.addBody(star) >= 0, "addBody(star)");

    float view[16], proj[16];
    translate(view, 0.0f, -40.0f, -90.0f);
    perspective(proj,
        50.0f * static_cast<float>(M_PI) / 180.0f,
        1.0f, 1.0f, 1000.0f);

    // ---- Render across 10 frames, alternating wireframe ----
    bool allPassed = true;
    const int numFrames = 10;
    for (int frame = 0; frame < numFrames; ++frame) {
        glViewport(0, 0, FB_W, FB_H);
        glClearColor(bg[0] / 255.0f, bg[1] / 255.0f,
                     bg[2] / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // The test FBO must be bound after glClear.
        checkState("After Clear (frame bound)");

        // Toggle wireframe every other frame to stress the polygon-mode save/restore.
        curv.setWireframe((frame % 2) == 1);
        curv.render(view, proj);
        allPassed &= checkState("After CurvatureRenderer::render");

        bodyRenderer.render(view, proj);
        allPassed &= checkState("After CelestialBodyRenderer::render");

        // Polygon mode must be GL_FILL — never leaked to GL_LINE.
        GLint polyMode = -1;
        glGetIntegerv(GL_POLYGON_MODE, &polyMode);
        bool polyOk = (polyMode == GL_FILL);
        check(polyOk,
            (std::string("Frame ") + std::to_string(frame)
             + ": glPolygonMode == GL_FILL").c_str());
        allPassed &= polyOk;

        // FBO binding must still be our test FBO.
        GLint fboAfter = -1;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fboAfter);
        bool fboOk = (fboAfter == static_cast<GLint>(testFBO));
        check(fboOk,
            (std::string("Frame ") + std::to_string(frame)
             + ": FBO preserved").c_str());
        allPassed &= fboOk;
    }

    // Save a snapshot for golden-image diffing.
    std::vector<uint8_t> pixels(FB_W * FB_H * 4);
    glReadPixels(0, 0, FB_W, FB_H, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    // OpenGL origin is bottom-left; PNG is top-left.  Flip rows.
    for (int y = 0; y < FB_H / 2; ++y) {
        int yInv = FB_H - 1 - y;
        for (int x = 0; x < FB_W; ++x) {
            for (int c = 0; c < 4; ++c) {
                std::swap(pixels[(static_cast<size_t>(y) * FB_W + x) * 4 + c],
                          pixels[(static_cast<size_t>(yInv) * FB_W + x) * 4 + c]);
            }
        }
    }
    stbi_write_png("latest_viewport_state.png", FB_W, FB_H, 4,
                   pixels.data(), FB_W * 4);

    // ---- Cleanup ----
    glDeleteRenderbuffers(1, &testDepthRBO);
    glDeleteTextures(1, &testColorTex);
    glDeleteFramebuffers(1, &testFBO);
    ctx.doneCurrent();

    std::cout << "=== Viewport State Test: "
              << ((allPassed && g_failures == 0) ? "ALL CHECKS PASSED"
                                                 : "FAILURES PRESENT")
              << " (" << g_failures << " failure(s)) ===" << std::endl;
    return (allPassed && g_failures == 0) ? 0 : 1;
}

// ---------------------------------------------------------------------------
// Mock GL mode: validates CPU-side rendering logic when no GL context exists.
// ---------------------------------------------------------------------------
static int runMockGLMode()
{
    std::cout << "=== Viewport State Test (Mock GL Mode) ===" << std::endl;
    int failures = 0;

    // 1. CurvatureRenderer: wireframe state and grid generation
    {
        quantumverse::CurvatureRenderer curv(
            8, 50.0f, quantumverse::CurvatureMode::GRID_DEFORMATION);
        if (!curv.isWireframe()) {
            std::cout << "  [PASS] CurvatureRenderer starts in fill mode" << std::endl;
        } else {
            std::cerr << "  [FAIL] CurvatureRenderer should start in fill mode" << std::endl;
            ++failures;
        }
        curv.setWireframe(true);
        if (curv.isWireframe()) {
            std::cout << "  [PASS] setWireframe(true) -> isWireframe() == true" << std::endl;
        } else {
            std::cerr << "  [FAIL] setWireframe(true) did not set wireframe" << std::endl;
            ++failures;
        }
        curv.setWireframe(false);
        if (!curv.isWireframe()) {
            std::cout << "  [PASS] setWireframe(false) -> isWireframe() == false" << std::endl;
        } else {
            std::cerr << "  [FAIL] setWireframe(false) did not clear wireframe" << std::endl;
            ++failures;
        }
    }

    // 2. CurvatureRenderer: mode switching
    {
        quantumverse::CurvatureRenderer curv(
            6, 50.0f, quantumverse::CurvatureMode::GRID_DEFORMATION);
        if (curv.getMode() == quantumverse::CurvatureMode::GRID_DEFORMATION) {
            std::cout << "  [PASS] Default mode is GRID_DEFORMATION" << std::endl;
        } else {
            std::cerr << "  [FAIL] Default mode mismatch" << std::endl;
            ++failures;
        }
        curv.setMode(quantumverse::CurvatureMode::RIEMANN_COLOR);
        if (curv.getMode() == quantumverse::CurvatureMode::RIEMANN_COLOR) {
            std::cout << "  [PASS] Mode switched to RIEMANN_COLOR" << std::endl;
        } else {
            std::cerr << "  [FAIL] Mode switch failed" << std::endl;
            ++failures;
        }
    }

    // 3. CelestialBodyRenderer: body management
    {
        quantumverse::CelestialBodyRenderer bodyRenderer(
            quantumverse::CelestialBodyRenderer::QualityLevel::LOW);
        if (bodyRenderer.bodyCount() == 0) {
            std::cout << "  [PASS] No bodies initially" << std::endl;
        } else {
            std::cerr << "  [FAIL] Expected 0 bodies" << std::endl;
            ++failures;
        }
        quantumverse::CelestialBodyInstance planet;
        planet.objectId = "test";
        planet.radius = 10.0f;
        if (bodyRenderer.addBody(planet) >= 0) {
            std::cout << "  [PASS] addBody succeeded" << std::endl;
        } else {
            std::cerr << "  [FAIL] addBody failed" << std::endl;
            ++failures;
        }
        if (bodyRenderer.bodyCount() == 1) {
            std::cout << "  [PASS] One body after add" << std::endl;
        } else {
            std::cerr << "  [FAIL] Expected 1 body, got " << bodyRenderer.bodyCount() << std::endl;
            ++failures;
        }
        if (bodyRenderer.removeBody("test")) {
            std::cout << "  [PASS] removeBody succeeded" << std::endl;
        } else {
            std::cerr << "  [FAIL] removeBody failed" << std::endl;
            ++failures;
        }
        if (bodyRenderer.bodyCount() == 0) {
            std::cout << "  [PASS] No bodies after remove" << std::endl;
        } else {
            std::cerr << "  [FAIL] Expected 0 bodies after remove" << std::endl;
            ++failures;
        }
    }

    std::cout << "=== Mock GL Summary: "
              << (failures == 0 ? "ALL CHECKS PASSED" : "FAILURES PRESENT")
              << " (" << failures << " failure(s)) ===" << std::endl;
    return failures == 0 ? 0 : 1;
}
