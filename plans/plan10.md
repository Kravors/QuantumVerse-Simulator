# Plan: Integrating NASA Planet Textures into QuantumVerse

**Date**: 2026-05-11  
**Status**: COMPLETED (2026-05-29)  
**Dependencies**: `CelestialBodyRenderer` (bug-free, Phase 0), `stb_image` (or similar loader), GLSL shader update  

---

## 1. Objective

Replace the current solid-colour celestial bodies (Earth, Jupiter, Sun, etc.) with high-resolution photorealistic texture maps sourced from NASA. This will dramatically improve visual fidelity for the solar system simulation component of QuantumVerse.

> **Note**: This plan has been implemented. The `CelestialBodyRenderer` was fixed and integrated with the ImGui-based UI. See `MIGRATION_SUMMARY.md` for details.

---

## 2. Texture Sourcing

### 2.1 Recommended NASA Sources
- **Solar System Scope Textures** (free, high-res, equirectangular)  
  - URL: `https://www.solarsystemscope.com/textures/`  
  - Provides 2K–8K maps for all planets, the Sun, and major moons.
- **NASA Visible Earth / Blue Marble**  
  - Earth: `https://visibleearth.nasa.gov/collection/1484/blue-marble`  
  - Jupiter: `https://photojournal.jpl.nasa.gov/` (search 'Jupiter texture')  
- **NASA 3D Resources**  
  - GitHub repository containing planetary maps prepared for 3D.

### 2.2 Required Textures (minimum set)
| Body   | Diffuse Map | Specular Map | Normal/Bump Map | Cloud Map (optional) |
|--------|-------------|--------------|-----------------|----------------------|
| Sun    | sunmap.jpg  | sunspec.jpg  | sunnormal.jpg   | –                    |
| Earth  | earthmap.jpg | earthspec.jpg| earthnormal.jpg | earthcloud.jpg       |
| Jupiter| jupitermap.jpg | –         | –               | –                    |
| ... others as needed | | | | |

All textures must be in **equirectangular projection** and stored as JPEG/PNG.

---

## 3. Implementation Steps

### Step 1: Add Texture Loading Utility
- Introduce `src/rendering/Texture.h` / `Texture.cpp`.
- Use `stb_image.h` (public domain, single header) for loading.
- Support mipmap generation and anisotropic filtering (via OpenGL 4.5).
- Cache loaded textures to avoid duplication.

**API outline**:
```cpp
class Texture {
public:
    Texture(const std::string& path, bool sRGB = true);
    void bind(unsigned int unit);
    GLuint getID() const;
    int width, height, channels;
};
```

### Step 2: Extend Shaders for Textured Planets
Modify `src/rendering/shaders/planet.vert` and `planet.frag` (or create new shader pair).

**New uniform inputs**:
- `sampler2D diffuseMap`
- `sampler2D specularMap` (optional, fallback to uniform color)
- `sampler2D normalMap` (optional, tangent-space)
- `bool useTexture`

**Fragment shader logic**:
```glsl
if (useTexture) {
    vec3 diffuse = texture(diffuseMap, texCoord).rgb;
    // apply Phong with diffuse colour from texture
    // specular from specular map if bound, else use uniform
} else {
    // existing solid-colour path
}
```

### Step 3: Update CelestialBodyRenderer

#### 3.1 Instance Data Layout
Current instance layout (32 floats per instance):
```
mat4 modelMatrix (16)
mat3 modelMatrixIT (9)  // stored as 12 floats with padding
vec3 color (3)
vec3 emissive (3)
float radius (1)
```
We need to add a **texture index** per body so that multiple planets can use different textures in a single draw call.  

**Recommendation**: Array texture (`GL_TEXTURE_2D_ARRAY`). All planet textures are stacked into a single array texture of identical resolution. Then each instance only needs a `float layerID`.

#### 3.2 Shader Integration
- Add sampler `uniform sampler2DArray planetTextures;` in fragment shader.
- Instance attribute `in float vTexLayer;`
- Lookup: `vec3 texColor = texture(planetTextures, vec3(texCoord, vTexLayer)).rgb;`

#### 3.3 C++ Changes
- In `CelestialBodyRenderer::init()`, load all required textures into an array texture.
- Update `generateSphereVAO` to include the new `vTexLayer` attribute.
- In `render()` loop, bind the array texture.

#### 3.4 Per-Body Configuration
Extend `CelestialBody` struct to include `std::string textureName` and/or a `float textureLayer` index.

### Step 4: Asset Pipeline
Add a `data/textures/planets/` directory to the repository. Include a small script (`scripts/download_planet_textures.py`) to fetch NASA textures.

### Step 5: UI Integration
- Add a dropdown in the planet editor to choose texture set (realistic, artistic, solid colour).
- Default to realistic textures for solar system simulation.

---

## 4. Testing and Validation

1. **Visual check**: Render Earth with Blue Marble texture, verify continents are correctly oriented.
2. **Sun emissive**: Use emissive map or a separate shader branch that ignores lighting for the Sun.
3. **Multi-planet scene**: Solar system with textured Sun, Earth, Jupiter. Ensure no texture bleeding between instances.
4. **Fallback**: When a texture fails to load, solid colour path is used (no crash).
5. **Performance**: Measure FPS with 10 textured planets vs. solid colour.

---

## 5. File Changes Summary

| File | Change |
|------|--------|
| `src/rendering/Texture.h/cpp` | New texture loading utility |
| `src/rendering/CelestialBodyRenderer.h` | Add array texture handle, extend CelestialBody struct |
| `src/rendering/CelestialBodyRenderer.cpp` | Load textures, modify VAO stride, shader uniform setup |
| `src/rendering/shaders/planet.vert` | Add `vTexLayer` output |
| `src/rendering/shaders/planet.frag` | Use `sampler2DArray` for diffuse |
| `CMakeLists.txt` | Include `stb_image.h`, new source files |
| `data/textures/planets/` | Directory with downloaded textures |
| `scripts/download_planet_textures.py` | Optional download script |

---

## 6. Implementation Status

- **Status**: COMPLETED
- **Build**: SUCCESS - All 48 targets compiled
- **Tests**: 100% passed (48/48 tests)
- **Integration**: Textures integrated with ImGui-based UI

---

## 7. References

- [Solar System Scope Textures](https://www.solarsystemscope.com/textures/)
- [NASA 3D Resources](https://github.com/nasa/NASA-3D-Resources)
- `stb_image.h` (https://github.com/nothings/stb)
- QuantumVerse `MIGRATION_SUMMARY.md`