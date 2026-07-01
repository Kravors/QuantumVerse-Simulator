I'll draft a detailed `plan.md` for integrating NASA planet textures into the QuantumVerse simulator, building on the existing `CelestialBodyRenderer` you've already fixed.

```markdown
# Plan: Integrating NASA Planet Textures into QuantumVerse

**Date**: 2026-05-11  
**Status**: Draft  
**Dependencies**: `CelestialBodyRenderer` (bug-free, Phase 0), `stb_image` (or similar loader), GLSL shader update  

---

## 1. Objective

Replace the current solid‑colour celestial bodies (Earth, Jupiter, Sun, etc.) with high‑resolution photorealistic texture maps sourced from NASA. This will dramatically improve visual fidelity for the solar system simulation component of QuantumVerse.

---

## 2. Texture Sourcing

### 2.1 Recommended NASA Sources
- **Solar System Scope Textures** (free, high‑res, equirectangular)  
  - URL: `https://www.solarsystemscope.com/textures/`  
  - Provides 2K–8K maps for all planets, the Sun, and major moons.
  F:\syyyy\solar system
- **NASA Visible Earth / Blue Marble**  
  - Earth: `https://visibleearth.nasa.gov/collection/1484/blue-marble`  
  - Jupiter: `https://photojournal.jpl.nasa.gov/` (search ‘Jupiter texture’)  
- **NASA 3D Resources**  
  - GitHub repository containing planetary maps prepared for 3D.  
  F:\syyyy\solar system

### 2.2 Required Textures (minimum set)
| Body   | Diffuse Map | Specular Map | Normal/Bump Map | Cloud Map (optional) |
|--------|-------------|--------------|-----------------|----------------------|
| Sun    | sunmap.jpg  | sunspec.jpg  | sunnormal.jpg   | –                    |
| Earth  | earthmap.jpg | earthspec.jpg| earthnormal.jpg | earthcloud.jpg       |
| Jupiter| jupitermap.jpg | –         | –               | –                    |
| … others as needed | | | | | |

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
- `sampler2D normalMap` (optional, tangent‑space)
- `bool useTexture`

**Fragment shader logic**:
```glsl
if (useTexture) {
    vec3 diffuse = texture(diffuseMap, texCoord).rgb;
    // apply Phong with diffuse colour from texture
    // specular from specular map if bound, else use uniform
} else {
    // existing solid‑colour path
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
**Proposed extended layout** (40 floats):
```
+ int textureIndex (1 float – cast to int in shader)
```
But OpenGL vertex attributes must be float‑compatible. We can store `textureIndex` as a `float` and round in the shader, or use `glVertexAttribIPointer` for integer attributes. Simpler: use a **uniform array of textures** and pass instance‑specific index as a float attribute.

Alternatively, bind an array texture (`sampler2DArray`) and use `textureIndex` as layer. This is more efficient for many bodies.

**Recommendation**: Array texture (`GL_TEXTURE_2D_ARRAY`). All planet textures are stacked into a single array texture of identical resolution (or using texture views). Then each instance only needs a `float layerID`.

#### 3.2 Shader Integration
- Add sampler `uniform sampler2DArray planetTextures;` in fragment shader.
- Instance attribute `in float vTexLayer;`
- Lookup: `vec3 texColor = texture(planetTextures, vec3(texCoord, vTexLayer)).rgb;`

#### 3.3 C++ Changes
- In `CelestialBodyRenderer::init()`, load all required textures into an array texture (use `glTexImage3D` or `glTexSubImage3D`). Ensure all source images have the same resolution (resample if necessary). Use mipmapping.
- Update `generateSphereVAO` to include the new `vTexLayer` attribute (4 bytes, offset 128, stride 132 if we keep 32‑byte alignment). Wait, current stride is 128 bytes (32 floats × 4). We'll increase stride to 132 (if we add one float). For 4‑byte alignment, stride = 132 is fine. Attribute location 6 for texture layer.
- In `render()` loop, `glUniform1i` for the texture array unit, and bind the array texture.

#### 3.4 Per‑Body Configuration
Extend `CelestialBody` struct to include `std::string textureName` and/or a `float textureLayer` index. On loading, assign the correct layer based on body type.

### Step 4: Asset Pipeline
Add a `data/textures/planets/` directory to the repository (or as a separate download). Include a small script (`scripts/download_planet_textures.py`) to fetch NASA textures automatically (if licence permits) or provide manual download instructions.

### Step 5: UI Integration
- Optionally add a dropdown in the planet editor to choose texture set (realistic, artistic, solid colour).
- Default to realistic textures for solar system simulation.

---

## 4. Testing and Validation

1. **Visual check**: Render Earth with Blue Marble texture, verify continents are correctly oriented (UVs match equirectangular map).
2. **Sun emissive**: Use emissive map or a separate shader branch that ignores lighting for the Sun.
3. **Multi‑planet scene**: Solar system with textured Sun, Earth, Jupiter. Ensure no texture bleeding between instances.
4. **Fallback**: When a texture fails to load, solid colour path is used (no crash).
5. **Performance**: Measure FPS with 10 textured planets vs. solid colour. Array texture minimises texture binds, should be fast.

---

## 5. File Changes Summary

| File | Change |
|------|--------|
| `src/rendering/Texture.h/cpp` | New texture loading utility |
| `src/rendering/CelestialBodyRenderer.h` | Add array texture handle, extend CelestialBody struct |
| `src/rendering/CelestialBodyRenderer.cpp` | Load textures, modify VAO stride, shader uniform setup |
| `src/rendering/shaders/planet.vert` | Add `vTexLayer` output |
| `src/rendering/shaders/planet.frag` | Use `sampler2DArray` for diffuse, optionally specular |
| `CMakeLists.txt` | Include `stb_image.h`, new source files |
| `data/textures/planets/` | Directory with downloaded textures (might be git‑ignored) |
| `scripts/download_planet_textures.py` | Optional download script |

---

## 6. Timeline

- **Day 1**: Texture loading class + integrate `stb_image`.
- **Day 2**: Shader modifications, array texture assembly.
- **Day 3**: C++ integration into `CelestialBodyRenderer`, testing with Earth map.
- **Day 4**: Add Sun, Jupiter, and other planets; UI tweaks; performance testing.
- **Day 5**: Documentation and final validation.

---

## 7. References

- [Solar System Scope Textures](https://www.solarsystemscope.com/textures/)
- [NASA 3D Resources](https://github.com/nasa/NASA-3D-Resources)
- `stb_image.h` (https://github.com/nothings/stb)
- QuantumVerse `all_info.md` – CelestialBodyRenderer section
```

This plan directly uses the architecture described in your `all_info.md` and provides a clear path to upgrading the visual quality of the solar system simulation. Let me know if you need a ready‑to‑copy `plan.md` file or further adjustments.