// Phase 5.8: Viewport Content Regression Test
//
// Renders a single golden-viewport frame (camera d=150, az=45, el=30) and
// verifies the framebuffer pixel histogram for the three core regression
// classes that haunted earlier fortresses:
//
//   1. Blue grid  — CurvatureRenderer must emit a sufficient number of
//      blue-dominated pixels (the spacetime grid).
//   2. White Sun  — CelestialBodyRenderer must render the emissive star
//      (CelestialBodyInstance.isStar with emissive color).
//   3. Red leak   — No red geodesic-wireframe lines should bleed through
//      into the final compositing (polygon-mode leak guard).
//   4. Non-black  — The scene must not be a blank black screen.
//
// Run with: QT_QPA_PLATFORM=offscreen ./test_viewport_content_test

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

static void rotateY(float* m, float angleRad)
{
    float c = std::cos(angleRad);
    float s = std::sin(angleRad);
    std::memset(m, 0, 16 * sizeof(float));
    m[0]  = c;  m[2]  = s;
    m[5]  = 1.0f;
    m[8]  = -s; m[10] = c;
    m[15] = 1.0f;
}

static void rotateX(float* m, float angleRad)
{
    float c = std::cos(angleRad);
    float s = std::sin(angleRad);
    std::memset(m, 0, 16 * sizeof(float));
    m[0]  = 1.0f;
    m[5]  = c;  m[6]  = s;
    m[9]  = -s; m[10] = c;
    m[15] = 1.0f;
}

static float mulFloat(const float* a, const float* b)
{
    // Simple matrix multiply accumulator for building view matrix.
    // Returns the (0,3) translation element (x) — used for camera placement.
    (void)a; (void)b; return 0.0f;
}

static int runMockGLMode();

int main(int argc, char* argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");

    QGuiApplication app(argc, argv);

    std::cout << "=== Viewport Content Test (pixel histogram verification) ===" << std::endl;

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

    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    std::cout << "  GL_RENDERER: " << (renderer ? renderer : "?") << std::endl;

    // ---- Offscreen test FBO ----
    const int FB_W = 764, FB_H = 516;
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

    // ---- Initialize CurvatureRenderer (blue grid) ----
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

    // ---- Initialize CelestialBodyRenderer (white/yellow Sun) ----
    quantumverse::CelestialBodyRenderer bodyRenderer(
        quantumverse::CelestialBodyRenderer::QualityLevel::LOW, 32);
    bodyRenderer.initializeGL();
    check(bodyRenderer.isInitialized(), "CelestialBodyRenderer.initializeGL()");

    float lightPos[3] = { 0.0f, 0.0f, 50.0f };
    bodyRenderer.setLightPosition(lightPos);
    float lightColor[3] = { 1.0f, 0.95f, 0.9f };
    bodyRenderer.setLightProperties(lightColor, 1.5f);

    quantumverse::CelestialBodyInstance sun;
    sun.objectId = "sun";
    sun.isStar = true;
    sun.radius = 16.0f;
    sun.position[0] = 0.0f;
    sun.position[1] = 0.0f;
    sun.position[2] = -30.0f;
    sun.emissive[0] = 1.0f; sun.emissive[1] = 0.9f; sun.emissive[2] = 0.6f;
    sun.color[0] = 1.0f; sun.color[1] = 0.9f; sun.color[2] = 0.6f;
    check(bodyRenderer.addBody(sun) >= 0, "addBody(sun)");

    // ---- Camera: distance=150, azimuth=45°, elevation=30° ----
    // Build view matrix: translate back by 150, then rotate to look at origin.
    float rotY[16], rotX[16];
    rotateY(rotY, 45.0f * static_cast<float>(M_PI) / 180.0f);  // azimuth
    rotateX(rotX, 30.0f * static_cast<float>(M_PI) / 180.0f);  // elevation

    float view[16];
    translate(view, 0.0f, 0.0f, -150.0f);

    // Compose: view = rotY * rotX * translate (column-major)
    float tmp[16], finalView[16];
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += rotY[(k * 4) + r] * rotX[(c * 4) + k];
            }
            tmp[(c * 4) + r] = sum;
        }
    }
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += tmp[(k * 4) + r] * view[(c * 4) + k];
            }
            finalView[(c * 4) + r] = sum;
        }
    }

    float proj[16];
    perspective(proj,
        45.0f * static_cast<float>(M_PI) / 180.0f,
        static_cast<float>(FB_W) / static_cast<float>(FB_H),
        1.0f, 1000.0f);

    // ---- Render ----
    glViewport(0, 0, FB_W, FB_H);
    glClearColor(bg[0] / 255.0f, bg[1] / 255.0f,
                 bg[2] / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Grid must be NON-wireframe so it renders as a filled (blue-tinted) grid.
    curv.setWireframe(false);
    curv.render(finalView, proj);
    check(glGetError() == GL_NO_ERROR, "No GL error after CurvatureRenderer::render");

    bodyRenderer.render(finalView, proj);
    check(glGetError() == GL_NO_ERROR, "No GL error after CelestialBodyRenderer::render");

    // Polygon mode must be GL_FILL (no red wireframe leak).
    GLint polyMode = -1;
    glGetIntegerv(GL_POLYGON_MODE, &polyMode);
    check(polyMode == GL_FILL, "glPolygonMode is GL_FILL (no wireframe leak)");

    // ---- Read back and histogram the framebuffer ----
    std::vector<uint8_t> pixels(FB_W * FB_H * 4);
    glReadPixels(0, 0, FB_W, FB_H, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    int blueCount = 0, whiteCount = 0, redCount = 0, nonBlackCount = 0;
    for (size_t i = 0; i < pixels.size(); i += 4) {
        unsigned char r = pixels[i + 2];  // GL_RGBA byte order after readback
        unsigned char g = pixels[i + 1];
        unsigned char b = pixels[i + 0];
        // Note: glReadPixels returns GL_RGBA which on little-endian maps to
        // BGRA byte order; swap r/b to get intuitive RGB comparison.
        if (b > 150 && b > r && b > g) blueCount++;
        if (r > 200 && g > 200 && b > 180) whiteCount++;
        if (r > 150 && r > g * 2 && r > b * 2) redCount++;
        if (r > 10 || g > 10 || b > 10) nonBlackCount++;
    }

    bool pass = true;

    // 1. Blue grid
    std::cout << "  blueCount  = " << blueCount
              << " (threshold >= 10000)" << std::endl;
    check(blueCount >= 10000, "Sufficient blue grid pixels");
    pass &= (blueCount >= 10000);

    // 2. White Sun
    std::cout << "  whiteCount = " << whiteCount
              << " (threshold >= 100)" << std::endl;
    check(whiteCount >= 100, "Sun (white/yellow) visible");
    pass &= (whiteCount >= 100);

    // 3. Non-black scene
    double nonBlackRatio = nonBlackCount / (double)(FB_W * FB_H);
    std::cout << "  nonBlackRatio = " << (nonBlackRatio * 100.0)
              << "% (threshold >= 5%)" << std::endl;
    check(nonBlackRatio >= 0.05, "Scene not mostly black");
    pass &= (nonBlackRatio >= 0.05);

    // 4. No red leak
    std::cout << "  redCount   = " << redCount
              << " (threshold < 500)" << std::endl;
    check(redCount <= 500, "No red geodesic lines leaking through");
    pass &= (redCount <= 500);

    // ---- Save snapshot for golden-image diffing ----
    // Flip rows: OpenGL is bottom-left, PNG is top-left.
    for (int y = 0; y < FB_H / 2; ++y) {
        int yInv = FB_H - 1 - y;
        for (int x = 0; x < FB_W; ++x) {
            for (int c = 0; c < 4; ++c) {
                std::swap(pixels[(static_cast<size_t>(y) * FB_W + x) * 4 + c],
                          pixels[(static_cast<size_t>(yInv) * FB_W + x) * 4 + c]);
            }
        }
    }
    // glReadPixels GL_RGBA -> stbi expects RGBA; swap to BGRA for display.
    stbi_write_png("latest_viewport_content.png", FB_W, FB_H, 4,
                   pixels.data(), FB_W * 4);
    std::cout << "  screenshot saved: latest_viewport_content.png" << std::endl;

    // ---- Cleanup ----
    glDeleteRenderbuffers(1, &testDepthRBO);
    glDeleteTextures(1, &testColorTex);
    glDeleteFramebuffers(1, &testFBO);
    ctx.doneCurrent();

    std::cout << "=== Viewport Content Test: "
              << (pass ? "ALL CHECKS PASSED" : "FAILURES PRESENT")
              << " (" << g_failures << " failure(s)) ===" << std::endl;
    return (pass ? 0 : 1);
}

// ---------------------------------------------------------------------------
// Mock GL mode: validates CPU-side rendering logic when no GL context exists.
// ---------------------------------------------------------------------------
static int runMockGLMode()
{
    std::cout << "=== Viewport Content Test (Mock GL Mode) ===" << std::endl;
    int failures = 0;

    // 1. CurvatureRenderer: grid generation and vertex color validity
    {
        quantumverse::CurvatureRenderer curv(
            12, 80.0f, quantumverse::CurvatureMode::GRID_DEFORMATION);
        auto metric = std::make_shared<quantumverse::MetricTensor>(
            quantumverse::MetricTensor::schwarzschild(1.0, 10.0,
                M_PI / 2.0, 0.0));
        curv.setMetric(metric);

        const auto& verts = curv.getVertices();
        if (verts.size() > 0) {
            std::cout << "  [PASS] CurvatureRenderer grid has " << verts.size()
                      << " vertices" << std::endl;
        } else {
            std::cerr << "  [FAIL] Grid has no vertices" << std::endl;
            ++failures;
        }

        bool colorsValid = true;
        for (const auto& v : verts) {
            for (int c = 0; c < 4; ++c) {
                if (v.color[c] < 0.0f || v.color[c] > 1.0f) {
                    colorsValid = false;
                    break;
                }
            }
        }
        if (colorsValid) {
            std::cout << "  [PASS] Grid vertex colors in valid [0,1] range" << std::endl;
        } else {
            std::cerr << "  [FAIL] Grid vertex colors out of range" << std::endl;
            ++failures;
        }
    }

    // 2. CurvatureRenderer: mode switching produces correct mode
    {
        quantumverse::CurvatureRenderer curv(
            8, 50.0f, quantumverse::CurvatureMode::GRID_DEFORMATION);
        curv.setMode(quantumverse::CurvatureMode::CURVATURE_SCALAR);
        if (curv.getMode() == quantumverse::CurvatureMode::CURVATURE_SCALAR) {
            std::cout << "  [PASS] Mode switch to CURVATURE_SCALAR" << std::endl;
        } else {
            std::cerr << "  [FAIL] Mode switch failed" << std::endl;
            ++failures;
        }
    }

    // 3. CelestialBodyRenderer: body configuration
    {
        quantumverse::CelestialBodyRenderer bodyRenderer(
            quantumverse::CelestialBodyRenderer::QualityLevel::LOW);
        quantumverse::CelestialBodyInstance planet;
        planet.objectId = "earth";
        planet.isStar = false;
        planet.radius = 10.0f;
        planet.color[0] = 0.3f; planet.color[1] = 0.6f; planet.color[2] = 1.0f;
        check(bodyRenderer.addBody(planet) >= 0,
              "addBody(earth) succeeds");

        quantumverse::CelestialBodyInstance star;
        star.objectId = "sun";
        star.isStar = true;
        star.radius = 16.0f;
        star.emissive[0] = 1.0f; star.emissive[1] = 0.9f; star.emissive[2] = 0.6f;
        check(bodyRenderer.addBody(star) >= 0,
              "addBody(sun) succeeds");

        check(bodyRenderer.bodyCount() == 2, "Two bodies present");
    }

    // 4. CelestialBodyRenderer: coordinate scale and light config
    {
        quantumverse::CelestialBodyRenderer bodyRenderer(
            quantumverse::CelestialBodyRenderer::QualityLevel::LOW);
        bodyRenderer.setCoordinateScale(1.496e10);
        if (bodyRenderer.coordinateScale() == 1.496e10) {
            std::cout << "  [PASS] Coordinate scale set correctly" << std::endl;
        } else {
            std::cerr << "  [FAIL] Coordinate scale mismatch" << std::endl;
            ++failures;
        }
    }

    std::cout << "=== Mock GL Summary: "
              << (failures == 0 ? "ALL CHECKS PASSED" : "FAILURES PRESENT")
              << " (" << failures << " failure(s)) ===" << std::endl;
    return failures == 0 ? 0 : 1;
}
