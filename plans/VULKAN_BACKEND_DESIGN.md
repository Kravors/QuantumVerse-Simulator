# QuantumVerse Vulkan Rendering Backend Design

**Status:** Design Document  
**Target:** Phase 1 - Architectural Modernisation  
**Date:** 2026-06-17

---

## 1. Executive Summary

This document outlines the design for a **Vulkan rendering backend** that will replace the current OpenGL implementation while maintaining full compatibility with the existing `UI4D_ImGui` interface. The design enables:

- Real-time 4D volumetric visualization via ray marching
- Compute-shader based curvature tensor evaluation
- Timeline-based frame graph for UI4D
- Cross-platform support (Windows/Linux/macOS)
- WebGPU target for WebAssembly compilation

---

## 2. Architecture Overview

### 2.1 Current State Analysis

The existing `CurvatureRenderer` uses:
- OpenGL 4.5 with vertex buffer objects (VBO/EBO)
- Shader-based grid deformation
- Manual framebuffer management via `Framebuffer` class
- Direct `ImGui::Image(ImTextureID, ...)` calls

### 2.2 Target Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    UI4D_ImGui (Unchanged)                  │
├─────────────────────────────────────────────────────────────┤
│              Rendering Abstraction Layer (RAL)               │
│  - RenderBackend (abstract interface)                     │
│  - VulkanBackend (Vulkan implementation)                   │
│  - OpenGLBackend (existing implementation)                  │
│  - WebGPUBackend (future WebAssembly target)                 │
├─────────────────────────────────────────────────────────────┤
│              CurvatureRenderer (Enhanced)                  │
│  - Uses RAL for all GPU operations                         │
│  - Compute shaders for tensor evaluation                   │
│  - Ray tracing for 4D visualization                        │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. Core Components

### 3.1 RenderBackend Interface

```cpp
// src/rendering/RenderBackend.h
#pragma once

#include <memory>
#include <vector>
#include <array>
#include <functional>

namespace quantumverse {
namespace rendering {

struct RenderBackendConfig {
    uint32_t width = 1920;
    uint32_t height = 1080;
    bool enableRayTracing = true;
    bool enableValidation = false;
};

class RenderBackend {
public:
    virtual ~RenderBackend() = default;
    
    // Lifecycle
    virtual bool initialize(const RenderBackendConfig& config) = 0;
    virtual void shutdown() = 0;
    virtual void resize(uint32_t width, uint32_t height) = 0;
    
    // Frame management
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    
    // Texture management
    virtual uint32_t createTexture(uint32_t width, uint32_t height, VkFormat format) = 0;
    virtual void destroyTexture(uint32_t textureId) = 0;
    virtual void* getTextureMemory(uint32_t textureId) = 0;
    
    // Framebuffer for ImGui integration
    virtual ImTextureID getImGuiTextureId(uint32_t textureId) = 0;
    
    // Compute dispatch
    virtual void dispatchCompute(uint32_t pipelineId, 
                               uint32_t x, uint32_t y, uint32_t z) = 0;
    
    // Ray tracing
    virtual void traceRays(uint32_t pipelineId,
                         const float* viewMatrix,
                         const float* projectionMatrix) = 0;
};

} // namespace rendering
} // namespace quantumverse
```

### 3.2 VulkanBackend Implementation

```cpp
// src/rendering/VulkanBackend.h
#pragma once

#include "RenderBackend.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace quantumverse {
namespace rendering {

class VulkanBackend : public RenderBackend {
public:
    bool initialize(const RenderBackendConfig& config) override;
    void shutdown() override;
    void resize(uint32_t width, uint32_t height) override;
    
    void beginFrame() override;
    void endFrame() override;
    
    uint32_t createTexture(uint32_t width, uint32_t height, VkFormat format) override;
    void destroyTexture(uint32_t textureId) override;
    void* getTextureMemory(uint32_t textureId) override;
    
    ImTextureID getImGuiTextureId(uint32_t textureId) override;
    
    void dispatchCompute(uint32_t pipelineId,
                      uint32_t x, uint32_t y, uint32_t z) override;
    
    void traceRays(uint32_t pipelineId,
                  const float* viewMatrix,
                  const float* projectionMatrix) override;

private:
    // Vulkan core objects
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    
    // Ray tracing pipeline
    VkPipeline rayTracingPipeline;
    VkShaderModule rayGenShader;
    VkShaderModule missShader;
    VkShaderModule closestHitShader;
    VkAccelerationStructureKHR topLevelAS;
    
    // Compute pipelines
    struct ComputePipeline {
        VkPipeline pipeline;
        VkShaderModule shader;
        VkDescriptorSetLayout descriptorSetLayout;
    };
    std::vector<ComputePipeline> computePipelines;
    
    // Descriptor pools
    VkDescriptorPool descriptorPool;
    
    // Framebuffers
    struct Framebuffer {
        VkImage image;
        VkImageView view;
        VkDeviceMemory memory;
        VkFramebuffer framebuffer;
        ImTextureID imguiId;
    };
    std::vector<Framebuffer> framebuffers;
    
    // Command buffers
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    
    // Synchronization
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    
    // Helper methods
    bool createInstance();
    bool pickPhysicalDevice();
    bool createDevice();
    bool createSwapchain();
    bool createRayTracingPipeline();
    bool createComputePipelines();
    bool createSyncObjects();
};

} // namespace rendering
} // namespace quantumverse
```

---

## 4. CurvatureRenderer Enhancement

### 4.1 Compute Shader for Tensor Evaluation

```glsl
// shaders/curvature_compute.comp
#version 460
#extension GL_EXT_shader_explicit_arithmetic_types : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_ray_query : require

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// Metric tensor storage buffer
layout(std430, binding = 0) restrict readonly buffer MetricBuffer {
    mat4x4 metrics[];
};

// Output curvature data
layout(std430, binding = 1) restrict writeonly buffer CurvatureBuffer {
    float kretschmann[];
    float ricciScalar[];
};

// Push constants for parameters
layout(push_constant) uniform PushConstants {
    vec3 gridOrigin;
    float gridSpacing;
    float time;
} pc;

// Christoffel symbols computation
void computeChristoffel(vec4 pos, out mat4 christoffel[4]) {
    // Implementation using metric derivatives
    // Γ^λ_μν = ½ g^λρ (∂_μ g_ρν + ∂_ν g_ρμ - ∂_ρ g_μν)
}

// Riemann tensor from Christoffel
void computeRiemann(in mat4 christoffel[4], out mat4x4 riemann[4]) {
    // R^ρ_σμν = ∂_μ Γ^ρ_νσ - ∂_ν Γ^ρ_μσ + Γ^ρ_μλ Γ^λ_νσ - Γ^ρ_νλ Γ^λ_μσ
}

void main() {
    uint idx = gl_GlobalInvocationID.x;
    uint idy = gl_GlobalInvocationID.y;
    
    vec4 position = vec4(
        pc.gridOrigin.x + idx * pc.gridSpacing,
        pc.gridOrigin.y + idy * pc.gridSpacing,
        0.0,
        pc.time
    );
    
    mat4 christoffel[4];
    computeChristoffel(position, christoffel);
    
    mat4x4 riemann[4];
    computeRiemann(christoffel, riemann);
    
    // Kretschmann scalar: K = R_ρσμν R^ρσμν
    float k = 0.0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                for (int l = 0; l < 4; l++) {
                    k += dot(riemann[i][j], riemann[k][l]);
                }
            }
        }
    }
    
    uint outputIdx = idx + idy * gl_NumWorkGroups.x;
    kretschmann[outputIdx] = k;
}
```

### 4.2 Ray Tracing for 4D Visualization

```glsl
// shaders/raytrace_4d.rgen
#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadEXT vec3 payload;

layout(binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 1) uniform UniformBuffer {
    mat4 view;
    mat4 proj;
    mat4 viewInv;
    mat4 projInv;
} ubo;

void main() {
    // Generate ray from camera through pixel
    vec2 pixel = vec2(gl_LaunchIDEXT.xy);
    vec2 screen = vec2(gl_LaunchSizeEXT.xy);
    
    vec2 uv = pixel / screen;
    vec4 ndc = vec4(uv * 2.0 - 1.0, -1.0, 1.0);
    
    vec4 rayOrigin = ubo.viewInv * vec4(0, 0, 0, 1);
    vec4 rayDir = ubo.viewInv * projInv * ndc;
    rayDir = normalize(rayDir);
    
    // Trace through 4D spacetime
    traceRayEXT(
        topLevelAS,
        gl_RayFlagsNoneEXT,
        0xFF,
        0,
        1,
        0,
        rayOrigin.xyz,
        0.001,
        rayDir.xyz,
        1000.0,
        0,
        0,
        0,
        payload
    );
}
```

---

## 5. Integration with UI4D_ImGui

### 5.1 Backend Selection

```cpp
// In UI4D_ImGui constructor
#ifdef QUANTUMVERSE_VULKAN_BACKEND
    renderBackend = std::make_unique<VulkanBackend>();
#else
    renderBackend = std::make_unique<OpenGLBackend>();
#endif
```

### 5.2 Framebuffer Integration

The `Framebuffer` class in `UI4D_ImGui.h` will be updated to use the backend:

```cpp
// Updated Framebuffer class
class Framebuffer {
public:
    void initialize(int w, int h) {
        width = w;
        height = h;
        textureId = renderBackend->createTexture(w, h, VK_FORMAT_R8G8B8A8_UNORM);
    }
    
    ImTextureID getImGuiTextureId() const {
        return renderBackend->getImGuiTextureId(textureId);
    }
    
private:
    uint32_t textureId = 0;
    int width = 0, height = 0;
    static RenderBackend* renderBackend;  // Injected at runtime
};
```

---

## 6. Build System Integration

### 6.1 CMake Configuration

```cmake
# CMakeLists.txt additions
option(QUANTUMVERSE_VULKAN_BACKEND "Use Vulkan backend" OFF)

if(QUANTUMVERSE_VULKAN_BACKEND)
    find_package(Vulkan REQUIRED)
    target_compile_definitions(quantumverse PRIVATE QUANTUMVERSE_VULKAN_BACKEND)
    target_link_libraries(quantumverse PRIVATE Vulkan::Vulkan)
    
    # Compile compute shaders
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/curvature_compute.spv
        COMMAND glslc -fshader-stage=compute 
                ${CMAKE_SOURCE_DIR}/shaders/curvature_compute.comp
                -o ${CMAKE_BINARY_DIR}/curvature_compute.spv
        DEPENDS ${CMAKE_SOURCE_DIR}/shaders/curvature_compute.comp
    )
endif()
```

### 6.2 Shader Compilation

```
shaders/
├── curvature_compute.comp     # Tensor evaluation
├── raytrace_4d.rgen           # Ray generation
├── raytrace_4d.rmiss          # Miss shader
├── raytrace_4d.rchit          # Closest hit
├── grid_vertex.vert           # Vertex shader
└── grid_fragment.frag         # Fragment shader
```

---

## 7. Performance Targets

| Operation | Current (OpenGL) | Target (Vulkan) | Improvement |
|-----------|------------------|-----------------|-------------|
| 1M curvature evaluations | ~500ms | ~50ms | 10x |
| 4D ray marching (1024x768) | N/A | <16ms | Real-time |
| Geodesic rendering (10K rays) | ~100ms | ~10ms | 10x |
| Memory bandwidth | CPU-GPU copy | Zero-copy | 5x |

---

## 8. Implementation Roadmap

### Week 1-2: Foundation
- [ ] Create `RenderBackend` abstract interface
- [ ] Implement `VulkanBackend` skeleton
- [ ] Add Vulkan SDK dependency to CMake
- [ ] Create basic triangle rendering test

### Week 3-4: Compute Shaders
- [ ] Implement curvature compute shader
- [ ] Integrate with `CurvatureRenderer`
- [ ] Add metric buffer upload
- [ ] Validate tensor calculations

### Week 5-6: Ray Tracing
- [ ] Create BLAS for grid geometry
- [ ] Create TLAS for 4D scene
- [ ] Implement ray generation shader
- [ ] Add intersection tests

### Week 7-8: UI Integration
- [ ] Update `Framebuffer` class
- [ ] Replace OpenGL calls in `UI4D_ImGui`
- [ ] Add backend selection logic
- [ ] Test with all 11 panels

### Week 9-10: Optimization
- [ ] Implement descriptor caching
- [ ] Add pipeline specialization
- [ ] Optimize memory barriers
- [ ] Profile and benchmark

---

## 9. Risk Mitigation

1. **Vulkan Complexity** - Use VKB::Framework as helper library
2. **ImGui Integration** - Use `imgui_impl_vulkan.cpp` from ImGui examples
3. **Cross-platform** - Test on Windows/Linux/macOS with MoltenVK
4. **Performance** - Profile with RenderDoc and Nsight Graphics

---

## 10. Success Criteria

- [ ] All existing tests pass with Vulkan backend
- [ ] 4D ray marching at 60 FPS (1024x768)
- [ ] Compute shader curvature evaluation < 50ms for 1M points
- [ ] Zero visual regression from OpenGL version
- [ ] WebGPU compilation target works

---

*Ready for implementation. The design maintains backward compatibility while enabling next-generation rendering capabilities.*