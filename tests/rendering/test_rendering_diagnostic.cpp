// QuantumVerse Rendering Diagnostic (GL-1281 investigation)
//
// Headless software-GL diagnostic for the celestial-body (planet/star
// texture) and curvature rendering paths. Creates an offscreen OpenGL
// context (Mesa llvmpipe via the per-app opengl32.dll drop-in), exercises
// both renderers, and reports GL errors plus FBO pixel readback so that
// silent rendering regressions and blank-output bugs become visible.
//
// Run with: QT_QPA_PLATFORM=offscreen ./test_rendering_diagnostic

// glad.h MUST be included before any Qt OpenGL header, or it errors with
// "OpenGL header already included". The renderer .cpp files follow the same
// rule; we mirror it here for the diagnostic harness.
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

#include "rendering/CelestialBodyRenderer.h"
#include "rendering/CurvatureRenderer.h"
#include "rendering/ProceduralTextures.h"
#include "spacetime/MetricTensor.h"

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

static bool readbackNonEmpty(int w, int h,
                              const uint8_t bg[4], int tol)
{
    std::vector<uint8_t> px(static_cast<size_t>(w) * h * 4);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    int nonEmpty = 0;
    for (int i = 0; i < w * h; ++i) {
        const uint8_t* p = &px[static_cast<size_t>(i) * 4];
        if (std::abs(p[0] - bg[0]) > tol || std::abs(p[1] - bg[1]) > tol ||
            std::abs(p[2] - bg[2]) > tol) {
            ++nonEmpty;
        }
    }
    std::cout << "    pixels rendered (non-background): " << nonEmpty
              << " / " << (w * h) << std::endl;
    return nonEmpty > 0;
}

int main(int argc, char* argv[])
{
    // Force the Qt offscreen platform so no display is required.
    qputenv("QT_QPA_PLATFORM", "offscreen");

    QGuiApplication app(argc, argv);

    std::cout << "=== Rendering Diagnostic (GL-1281) ===" << std::endl;

    // ---- Offscreen OpenGL context ----
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setVersion(4, 5);
    fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    fmt.setDepthBufferSize(24);

    QOffscreenSurface surface;
    surface.setFormat(fmt);
    surface.create();
    check(surface.isValid(), "QOffscreenSurface created");

    QOpenGLContext ctx;
    ctx.setFormat(fmt);
    if (!ctx.create()) {
        std::cerr << "  [SKIP] QOpenGLContext::create() failed: no usable software-GL on this platform.\n"
                  << "         This diagnostic runs on the project's Linux CI (xvfb + Mesa llvmpipe),\n"
                  << "         where it exercises both renderers and reports GL errors / blank output." << std::endl;
        return 0; // soft skip; non-fatal on platforms without a GL driver
    }
    ctx.makeCurrent(&surface);

    // ---- Load GLAD over the Qt context ----
    auto loader = [](const char* name) -> void* {
        QOpenGLContext* c = QOpenGLContext::currentContext();
        if (!c) return nullptr;
        return reinterpret_cast<void*>(c->getProcAddress(name));
    };
    if (!gladLoadGLLoader(loader)) {
        std::cerr << "  [SKIP] gladLoadGLLoader() failed: no usable software-GL on this platform." << std::endl;
        return 0;
    }
    check(true, "gladLoadGLLoader()");

    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    std::cout << "  GL_VERSION:  " << (version ? version : "?") << std::endl;
    std::cout << "  GL_RENDERER: " << (renderer ? renderer : "?") << std::endl;
    std::cout << "  GL_VENDOR:   " << (vendor ? vendor : "?") << std::endl;
    check(glGetError() == GL_NO_ERROR, "no GL error after context init");

    const bool usingSoftwareGL = renderer && std::strstr(renderer, "llvmpipe");
    std::cout << "  software-GL (llvmpipe): " << (usingSoftwareGL ? "yes" : "no") << std::endl;

    // ---- Offscreen default framebuffer for pixel readback ----
    const int FB_W = 512, FB_H = 512;
    uint8_t bg[4] = { 13, 13, 18, 255 }; // ~ (0.05,0.05,0.07)

    // ===================================================================
    // CelestialBodyRenderer: planet + star procedural textures
    // ===================================================================
    std::cout << "--- CelestialBodyRenderer (planet/star textures) ---" << std::endl;
    {
        quantumverse::CelestialBodyRenderer bodyRenderer(
            quantumverse::CelestialBodyRenderer::QualityLevel::MEDIUM);
        bodyRenderer.initializeGL();
        check(bodyRenderer.isInitialized(), "CelestialBodyRenderer.initializeGL()");

        check(bodyRenderer.loadTextureArray({}, 512, 256),
              "loadTextureArray (1 procedural layer)");

        quantumverse::PlanetTextureConfig planetCfg;
        planetCfg.type = quantumverse::PlanetTextureConfig::PlanetType::TERRESTRIAL;
        planetCfg.width = 512; planetCfg.height = 256; planetCfg.seed = 7;
        check(bodyRenderer.generateProceduralTexture(0, planetCfg),
              "generateProceduralTexture (planet layer 0)");

        float lpos[3] = { 0.0f, 60.0f, 120.0f };
        bodyRenderer.setLightPosition(lpos);
        const float lightColor[3] = { 1.0f, 1.0f, 1.0f };
        bodyRenderer.setLightProperties(lightColor, 1.5f);

        quantumverse::CelestialBodyInstance planet;
        planet.objectId = "earth";
        planet.radius = 10.0f;
        planet.position[0] = -25.0f;
        planet.textureLayer = 0;
        planet.color[0] = planet.color[1] = planet.color[2] = 1.0f;
        check(bodyRenderer.addBody(planet) >= 0, "addBody(planet)");

        quantumverse::CelestialBodyInstance star;
        star.objectId = "sun";
        star.isStar = true;
        star.radius = 16.0f;
        star.position[0] = 25.0f;
        star.textureLayer = -1;
        star.emissive[0] = 1.0f; star.emissive[1] = 0.85f; star.emissive[2] = 0.4f;
        star.color[0] = 1.0f; star.color[1] = 0.9f; star.color[2] = 0.6f;
        check(bodyRenderer.addBody(star) >= 0, "addBody(star)");

        glViewport(0, 0, FB_W, FB_H);
        glClearColor(bg[0] / 255.0f, bg[1] / 255.0f, bg[2] / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        float view[16], proj[16];
        translate(view, 0.0f, 0.0f, -120.0f);
        perspective(proj, 50.0f * static_cast<float>(M_PI) / 180.0f, 1.0f, 1.0f, 1000.0f);
        bodyRenderer.render(view, proj);
        check(glGetError() == GL_NO_ERROR, "CelestialBodyRenderer.render() no GL error");
        check(readbackNonEmpty(FB_W, FB_H, bg, 12),
              "CelestialBody rendered non-empty output");
    }

    // ===================================================================
    // CurvatureRenderer: Schwarzschild grid deformation
    // ===================================================================
    std::cout << "--- CurvatureRenderer (Schwarzschild grid) ---" << std::endl;
    {
        // Use a modest grid resolution: under software-GL (llvmpipe) CI the
        // per-vertex deformation vs. full Kretschmann is O(res^3) and a 64^3
        // grid times out the test. 24^3 still exercises the full pipeline.
        quantumverse::CurvatureRenderer curv(
            24, 100.0f, quantumverse::CurvatureMode::GRID_DEFORMATION);
        curv.initializeGL();
        check(curv.isInitialized(), "CurvatureRenderer.initializeGL()");

        auto metric = std::make_shared<quantumverse::MetricTensor>(
            quantumverse::MetricTensor::schwarzschild(1.0, 10.0, M_PI / 2.0, 0.0));
        curv.setMetric(metric);

        glViewport(0, 0, FB_W, FB_H);
        glClearColor(bg[0] / 255.0f, bg[1] / 255.0f, bg[2] / 255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        float view[16], proj[16];
        translate(view, 0.0f, -40.0f, -90.0f);
        perspective(proj, 50.0f * static_cast<float>(M_PI) / 180.0f, 1.0f, 1.0f, 1000.0f);
        curv.render(view, proj);
        check(glGetError() == GL_NO_ERROR, "CurvatureRenderer.render() no GL error");
        check(readbackNonEmpty(FB_W, FB_H, bg, 12),
              "Curvature grid rendered non-empty output");

        // Also exercise the Riemann-color and curvature-scalar shader paths.
        curv.setMode(quantumverse::CurvatureMode::RIEMANN_COLOR);
        glClear(GL_COLOR_BUFFER_BIT);
        curv.render(view, proj);
        check(glGetError() == GL_NO_ERROR, "CurvatureRenderer RIEMANN_COLOR no GL error");

        curv.setMode(quantumverse::CurvatureMode::CURVATURE_SCALAR);
        glClear(GL_COLOR_BUFFER_BIT);
        curv.render(view, proj);
        check(glGetError() == GL_NO_ERROR, "CurvatureRenderer CURVATURE_SCALAR no GL error");
    }

    ctx.doneCurrent();

    std::cout << "=== Diagnostic summary: "
              << (g_failures == 0 ? "ALL CHECKS PASSED" : "FAILURES PRESENT")
              << " (" << g_failures << " failure(s)) ===" << std::endl;
    return g_failures == 0 ? 0 : 1;
}
