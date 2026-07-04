/**
 * @file QuantumGeometryRenderer.cpp
 * @brief Quantum Geometry Visualizer - Phase 2 stub implementation
 *
 * Quantum geometry rendering is a Phase 2 feature. All engine-dependent
 * methods are stubs until CDT/SpinFoam/GFT/CausalSet engines are fully integrated.
 */

#include "QuantumGeometryRenderer.h"
#include "../ui4d/UI4D.h"  // For full Camera4D definition
#include <cmath>
#include <algorithm>

namespace quantumverse {

QuantumGeometryRenderer::QuantumGeometryRenderer()
     : currentType(QuantumGeometryType::NONE),
       showWireframe(true),
       showVolumes(false),
       showLabels(false),
       animateEvolution(false),
       animationTime(0.0),
       currentFrame(0),
       targetLOD(3)
{
    lodLevels = {
        {1000,  2.0f, 1.0f, false},
        {5000,  3.0f, 1.5f, false},
        {20000, 4.0f, 2.0f, true},
        {100000, 5.0f, 3.0f, true}
    };
}

void QuantumGeometryRenderer::setCDTEngine(const std::shared_ptr<CDTEngine>& /*engine*/)
{
    currentType = QuantumGeometryType::CDT;
}

void QuantumGeometryRenderer::setSpinFoamEngine(const std::shared_ptr<SpinFoamEngine>& /*engine*/)
{
    currentType = QuantumGeometryType::SPIN_FOAM;
}

void QuantumGeometryRenderer::setGFTEngine(const std::shared_ptr<GFTEngine>& /*engine*/)
{
    currentType = QuantumGeometryType::GFT;
}

void QuantumGeometryRenderer::setCausalSetEngine(const std::shared_ptr<CausalSetEngine>& /*engine*/)
{
    currentType = QuantumGeometryType::CAUSAL_SET;
}

void QuantumGeometryRenderer::clearGeometry()
{
    currentType = QuantumGeometryType::NONE;
    quantumSource.reset();
}

void QuantumGeometryRenderer::setViewScale(double scale_meters)
{
    currentScale = scale_meters;
    updateLODFromScale();
}

void QuantumGeometryRenderer::render(const Camera4D& camera,
                                     const std::vector<SliceView>& /*slices*/,
                                     const MetricTensor& /*metric*/)
{
    // Use parent class's isInitialized() method
    if (!isInitialized()) return;
    
    // Get view and projection matrices from camera
    auto viewMatrix = camera.computeViewMatrix4x4();
    auto projMatrix = camera.computeProjectionMatrix4x4(1.0);  // aspect ratio 1.0
    
    // Render based on current quantum geometry type
    switch (currentType) {
        case QuantumGeometryType::CDT:
            renderCDTGeometry(viewMatrix, projMatrix);
            break;
        case QuantumGeometryType::SPIN_FOAM:
            renderSpinFoamGeometry(viewMatrix, projMatrix);
            break;
        case QuantumGeometryType::GFT:
            renderGFTGeometry(viewMatrix, projMatrix);
            break;
        case QuantumGeometryType::CAUSAL_SET:
            renderCausalSetGeometry(viewMatrix, projMatrix);
            break;
        default:
            // No quantum geometry - render classical curvature
            // Convert double arrays to float arrays for parent render()
            static thread_local float viewMatrixF[16];
            static thread_local float projMatrixF[16];
            for (int i = 0; i < 16; i++) {
                viewMatrixF[i] = static_cast<float>(viewMatrix[i]);
                projMatrixF[i] = static_cast<float>(projMatrix[i]);
            }
            CurvatureRenderer::render(viewMatrixF, projMatrixF);
            break;
    }
}

void QuantumGeometryRenderer::renderCDTGeometry(const std::array<double, 16>& viewMatrix,
                                                 const std::array<double, 16>& projMatrix)
{
    // Render CDT simplices (4-simplices as tetrahedra)
    // For now, render a simple wireframe grid to represent discrete geometry
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    // Use the grid shader from parent class via protected getter
    ShaderProgram* shader = &getGridShader();
    shader->use();
    shader->setUniform("viewMatrix", viewMatrix.data());
    shader->setUniform("projectionMatrix", projMatrix.data());
    shader->setUniform("time", static_cast<float>(animationTime));
    shader->setUniform("curvatureScale", 1.0f);
    shader->setUniform("curvatureMode", static_cast<int>(CurvatureMode::GRID_DEFORMATION));
    
    // Render vertices as points for CDT
    if (auto it = geometryCache.find(QuantumGeometryType::CDT); it != geometryCache.end()) {
        const GeometryBuffers& buffers = it->second;
        if (buffers.isValid && buffers.numVertices > 0) {
            glPointSize(lodLevels[targetLOD].pointSize);
            glBindVertexArray(buffers.vao);
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(buffers.numVertices));
            glBindVertexArray(0);
        }
    }
    
    shader->release();
}

void QuantumGeometryRenderer::renderSpinFoamGeometry(const std::array<double, 16>& viewMatrix,
                                                     const std::array<double, 16>& projMatrix)
{
    // Render spin network edges
    // Vertices as spheres, edges as cylinders
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    ShaderProgram* shader = &getGridShader();
    shader->use();
    shader->setUniform("viewMatrix", viewMatrix.data());
    shader->setUniform("projectionMatrix", projMatrix.data());
    shader->setUniform("time", static_cast<float>(animationTime));
    shader->setUniform("curvatureScale", 1.0f);
    shader->setUniform("curvatureMode", static_cast<int>(CurvatureMode::RIEMANN_COLOR));
    
    // Render vertices
    if (auto it = geometryCache.find(QuantumGeometryType::SPIN_FOAM); it != geometryCache.end()) {
        const GeometryBuffers& buffers = it->second;
        if (buffers.isValid && buffers.numVertices > 0) {
            glPointSize(lodLevels[targetLOD].pointSize);
            glBindVertexArray(buffers.vao);
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(buffers.numVertices));
            glBindVertexArray(0);
        }
    }
    
    shader->release();
}

void QuantumGeometryRenderer::renderGFTGeometry(const std::array<double, 16>& viewMatrix,
                                                 const std::array<double, 16>& projMatrix)
{
    // Render GFT field as isosurfaces
    // Color by field magnitude |φ|
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    ShaderProgram* shader = &getCurvatureShader();
    shader->use();
    shader->setUniform("viewMatrix", viewMatrix.data());
    shader->setUniform("projectionMatrix", projMatrix.data());
    shader->setUniform("time", static_cast<float>(animationTime));
    shader->setUniform("curvatureScale", 1.0f);
    shader->setUniform("curvatureMode", static_cast<int>(CurvatureMode::CURVATURE_SCALAR));
    
    if (auto it = geometryCache.find(QuantumGeometryType::GFT); it != geometryCache.end()) {
        const GeometryBuffers& buffers = it->second;
        if (buffers.isValid && buffers.numIndices > 0) {
            glBindVertexArray(buffers.vao);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(buffers.numIndices),
                         GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }
    
    shader->release();
}

void QuantumGeometryRenderer::renderCausalSetGeometry(const std::array<double, 16>& viewMatrix,
                                                     const std::array<double, 16>& projMatrix)
{
    // Render causal set as point cloud with causal links
    // Points for sprinkled events, lines for causal relations
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    ShaderProgram* shader = &getGridShader();
    shader->use();
    shader->setUniform("viewMatrix", viewMatrix.data());
    shader->setUniform("projectionMatrix", projMatrix.data());
    shader->setUniform("time", static_cast<float>(animationTime));
    shader->setUniform("curvatureScale", 1.0f);
    shader->setUniform("curvatureMode", static_cast<int>(CurvatureMode::TIME_DILATION));
    
    if (auto it = geometryCache.find(QuantumGeometryType::CAUSAL_SET); it != geometryCache.end()) {
        const GeometryBuffers& buffers = it->second;
        if (buffers.isValid && buffers.numVertices > 0) {
            glPointSize(lodLevels[targetLOD].pointSize);
            glBindVertexArray(buffers.vao);
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(buffers.numVertices));
            glBindVertexArray(0);
        }
    }
    
    shader->release();
}

void QuantumGeometryRenderer::renderQuantumGeometry(const Camera4D& /*camera*/)
{
    // Phase 2: render cached geometry
}

void QuantumGeometryRenderer::renderSimplex(const std::array<Event4D, 4>& /*vertices*/,
                                            const std::array<double, 4>& /*colors*/)
{
    // Phase 2: render 4-simplex wireframe
}

void QuantumGeometryRenderer::renderSpinEdge(const Event4D& /*v1*/, const Event4D& /*v2*/,
                                             double /*spin*/, const double[/*4*/] /*color*/)
{
    // Phase 2: render spin network edge
}

QuantumGeometryRenderer::GeometryInfo QuantumGeometryRenderer::getGeometryInfo() const
{
    GeometryInfo info;
    info.type = currentType;
    info.vertexCount = 0;
    info.edgeCount = 0;
    info.simplexCount = 0;

    switch (currentType) {
        case QuantumGeometryType::CDT:
            info.description = "Causal Dynamical Triangulations: 4D simplices (Phase 2)";
            break;
        case QuantumGeometryType::SPIN_FOAM:
            info.description = "Spin Foam: SU(2) spin network (Phase 2)";
            break;
        case QuantumGeometryType::GFT:
            info.description = "Group Field Theory: field condensate (Phase 2)";
            break;
        case QuantumGeometryType::CAUSAL_SET:
            info.description = "Causal Set: discrete poset (Phase 2)";
            break;
        default:
            info.description = "None";
    }

    return info;
}

void QuantumGeometryRenderer::setLODLevel(int level)
{
    targetLOD = std::max(0, std::min(3, level));
}

void QuantumGeometryRenderer::stepAnimation()
{
    currentFrame++;
    animationTime += 0.033;
}

void QuantumGeometryRenderer::resetAnimation()
{
    currentFrame = 0;
    animationTime = 0.0;
}

void QuantumGeometryRenderer::updateColorScheme()
{
    // Phase 2: recompute colors for cached geometries
}

void QuantumGeometryRenderer::updateLODFromScale()
{
    double logScale = std::log10(currentScale > 0 ? currentScale : 1.0);
    double norm = (logScale + 35.0) / 35.0;
    targetLOD = static_cast<int>((1.0 - norm) * 4.0);
    targetLOD = std::max(0, std::min(3, targetLOD));
}

void QuantumGeometryRenderer::animateGeometry(double /*deltaTime*/)
{
    animationTime += 0.033;
}

std::array<float, 3> QuantumGeometryRenderer::projectToScreen(const Event4D& point,
                                                               const Camera4D& /*camera*/,
                                                               int /*viewportWidth*/,
                                                               int /*viewportHeight*/) const
{
    std::array<float, 3> result;
    result[0] = static_cast<float>(point.x);
    result[1] = static_cast<float>(point.y);
    result[2] = static_cast<float>(point.z);
    return result;
}

std::array<float, 4> QuantumGeometryRenderer::colorFromValue(double value,
                                                              double minVal,
                                                              double maxVal,
                                                              ColorScheme /*scheme*/) const
{
    double t = (value - minVal) / (maxVal - minVal + 1e-10);
    t = std::max(0.0, std::min(1.0, t));

    std::array<float, 4> color = {1.0f, 1.0f, 1.0f, 1.0f};
    if (t < 0.5) {
        color = {0.0f, static_cast<float>(2*t), 1.0f - static_cast<float>(2*t), 1.0f};
    } else {
        color = {static_cast<float>(2*(t-0.5)), 1.0f - static_cast<float>(2*(t-0.5)), 0.0f, 1.0f};
    }
    return color;
}

void QuantumGeometryRenderer::uploadGeometryToGPU(
    GeometryBuffers& buffers,
    const std::vector<double>& /*vertices*/,
    const std::vector<int>& /*indices*/,
    const std::vector<double>& /*colors*/)
{
    buffers.isValid = false;
}

void QuantumGeometryRenderer::generateCDTGeometry(const std::vector<double>&,
                                                   const std::vector<int>&) {}
void QuantumGeometryRenderer::generateSpinFoamGeometry(const std::vector<double>&,
                                                        const std::vector<int>&,
                                                        const std::vector<double>&) {}
void QuantumGeometryRenderer::generateGFTGeometry(const std::vector<double>&,
                                                   int) {}
void QuantumGeometryRenderer::generateCausalSetGeometry(const std::vector<double>&,
                                                         const std::vector<int>&,
                                                         const std::vector<double>&) {}

} // namespace quantumverse