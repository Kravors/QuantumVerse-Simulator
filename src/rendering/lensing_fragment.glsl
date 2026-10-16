#version 450 core

in vec2 vTexCoord;
out vec4 outColor;

// Black hole parameters
uniform float u_mass;
uniform float u_spin;
uniform int u_raySteps;
uniform float u_maxDistance;
uniform float u_shadowIntensity;
uniform float u_starFieldIntensity;
uniform float u_photonRingIntensity;
uniform float u_accretionDiskIntensity;
uniform bool u_enablePhotonRing;
uniform bool u_enableAccretionDisk;
uniform bool u_enableStarField;

// Volumetric accretion disk parameters
uniform bool u_volumetricDiskEnable;
uniform float u_volumetricDiskDensity;
uniform float u_volumetricDiskTemperature;
uniform float u_volumetricDiskScaleHeight;
uniform float u_volumetricDiskInner;
uniform float u_volumetricDiskOuter;
uniform int u_volumetricDiskSteps;
uniform float u_volumetricDiskOpacity;
uniform float u_volumetricDiskIntensity;
uniform float u_volumetricDiskDopplerBoost;
uniform float u_volumetricDiskInnerEdgeFade;

// Camera parameters
uniform vec3 u_cameraPos;
uniform vec3 u_cameraDir;
uniform vec3 u_cameraUp;
uniform float u_fov;

// Star field
uniform samplerCube u_starField;

const float PI = 3.14159265358979323846;
const float TWO_PI = 6.28318530717958647692;

// Approximate blackbody RGB from effective temperature.
// T=1.0 maps to a neutral tint (sum=1 so total intensity is unchanged);
// T<1 shifts warm (red/orange), T>1 shifts cool (blue/white).  The hue
// falls out from T_obs = T_emit * delta; the scalar T_obs^4 term in j
// carries the total luminosity.
vec3 blackbodyColor(float T) {
    float t = max(T, 0.01);
    vec3 color;
    color.r = 1.0 / (1.0 + exp(2.0 * (0.8 - t)));
    color.g = 1.0 / (1.0 + exp(3.0 * (1.0 - t)));
    color.b = 1.0 / (1.0 + exp(4.0 * (1.3 - t)));
    return color / (color.r + color.g + color.b);
}

// Compute Schwarzschild radius
float schwarzschildRadius(float mass) {
    return 2.0 * mass;
}

// Kerr metric: compute horizon radius
float kerrHorizon(float mass, float spin) {
    float M = mass;
    float a = spin * mass;
    return M + sqrt(max(M * M - a * a, 0.0));
}

// Kerr metric: compute ergosphere radius at given theta
float kerrErgosphere(float mass, float spin, float theta) {
    float M = mass;
    float a = spin * mass;
    return M + sqrt(max(M * M - a * a * cos(theta) * cos(theta), 0.0));
}

// Bardeen-Press-Teukolsky (1972) innermost stable circular orbit, in units of M.
// spin is the dimensionless a/M; positive => prograde (disk co-rotates).
// Schwarzschild limit (a=0) is exactly 6M; extremal prograde (a->1) -> M.
float computeISCO(float spin) {
    float a = clamp(abs(spin), 0.0f, 0.9999f);
    float a2 = a * a;

    // cbrt(1-a^2): guarded explicitly because some drivers return NaN for
    // pow(0, 1/3) at a->1 even though a is clamped below 1.
    float cbrt_1ma2 = (a2 >= 1.0f) ? 0.0f : pow(1.0f - a2, 1.0f / 3.0f);
    float cbrt1pa   = pow(1.0f + a, 1.0f / 3.0f);
    float cbrt1ma   = pow(1.0f - a, 1.0f / 3.0f);

    float Z1 = 1.0f + cbrt_1ma2 * (cbrt1pa + cbrt1ma);
    float Z2 = sqrt(3.0f * a2 + Z1 * Z1);

    float inner = sqrt(max((3.0f - Z1) * (3.0f + Z1 + 2.0f * Z2), 0.0f));
    float signFactor = (spin >= 0.0f) ? 1.0f : -1.0f;

    return 3.0f + Z2 - signFactor * inner;
}

// Procedural star field (fallback when texture not available)
vec3 proceduralStarField(vec3 dir) {
    // Hash function for star placement
    vec3 absDir = abs(dir);
    float scale = 50.0;
    vec3 grid = floor(dir * scale);
    vec3 frac = fract(dir * scale);

    // Simple hash
    float h = sin(dot(grid, vec3(127.1, 311.7, 74.7))) * 43758.5453;
    float star = fract(h);

    // Stars appear rarely
    vec3 color = vec3(0.0);
    if (star > 0.97) {
        float brightness = (star - 0.97) / 0.03;
        // Vary star colors (blue-white to red)
        float colorType = fract(h * 3.7);
        vec3 starColor;
        if (colorType < 0.3) {
            starColor = vec3(0.8, 0.9, 1.0);  // Blue-white
        } else if (colorType < 0.7) {
            starColor = vec3(1.0, 1.0, 0.95);  // White
        } else {
            starColor = vec3(1.0, 0.9, 0.7);  // Yellow-orange
        }
        color = starColor * brightness * u_starFieldIntensity;
    }

    // Subtle nebula background
    float nebula = sin(dir.x * 3.0 + dir.y * 5.0) * cos(dir.z * 7.0) * 0.5 + 0.5;
    color += vec3(0.01, 0.005, 0.02) * nebula * u_starFieldIntensity;

    return color;
}

// Sample star field (texture or procedural)
vec3 sampleStarField(vec3 dir) {
    if (!u_enableStarField) return vec3(0.0);
    vec3 texColor = texture(u_starField, dir).rgb;
    if (texColor.r > 0.0 || texColor.g > 0.0 || texColor.b > 0.0) {
        return texColor * u_starFieldIntensity;
    }
    return proceduralStarField(dir) * u_starFieldIntensity;
}

// Compute deflection due to gravity (strong-field null geodesic deviation)
// Includes both radial and tangential components.  The tangential term is
// what bends rays around the black hole, producing the Einstein ring.
// Weak-field limit: matches 2*r_s / b for large impact parameter b.
vec3 computeDeflection(vec3 pos, vec3 rayDir, float mass, float spin) {
    float r = length(pos);
    if (r < 0.001) return vec3(0.0);

    float rs = 2.0 * mass;
    vec3 rHat = pos / r;
    vec3 dir = normalize(rayDir);

    float invR2 = 1.0 / (r * r);
    float invR3 = invR2 / r;

    // Radial component (toward BH)
    float radialStrength = rs * invR2;

    // Tangential component (perpendicular to ray, toward BH in orbital plane)
    // This bends rays around the BH; without it no Einstein ring forms.
    float radialDot = dot(dir, rHat);
    vec3 tangentialDir = normalize(cross(dir, rHat));
    float tangentialStrength = rs * invR2 * abs(radialDot);

    vec3 deflection = -rHat * radialStrength;
    deflection += -tangentialDir * tangentialStrength;

    // Kerr frame-dragging correction
    if (spin > 0.001) {
        vec3 spinAxis = vec3(0.0, 1.0, 0.0);  // Spin along y-axis
        vec3 perpComponent = cross(spinAxis, pos);
        float twistStrength = spin * rs * rs * invR3;
        deflection += perpComponent * twistStrength;
    }

    return deflection;
}

// Orbit equation RHS: d²u/dφ² + u = 3 M u², u = 1/r
// State: [u, du/dφ], derivative: [du/dφ, 3*M*u² - u]
vec2 orbitRhs(vec2 state, float M) {
    float u = state.x;
    float up = state.y;
    return vec2(up, 3.0 * M * u * u - u);
}

// Single RK4 step on the orbit equation
vec2 orbitRK4Step(vec2 state, float dPhi, float M) {
    vec2 k1 = orbitRhs(state, M);
    vec2 k2 = orbitRhs(state + 0.5 * dPhi * k1, M);
    vec2 k3 = orbitRhs(state + 0.5 * dPhi * k2, M);
    vec2 k4 = orbitRhs(state + dPhi * k3, M);
    return state + (dPhi / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4);
}

// Ray march through curved spacetime using Schwarzschild geodesic integration
// (orbit equation).  Replaces the weak-field deflection march so that rays
// skimming the photon sphere are bent strongly enough to form an Einstein ring.
vec3 rayMarch(vec3 rayOrigin, vec3 rayDir) {
    float M = u_mass;
    float rs = 2.0 * M;
    float horizon = rs;

    // Orbital plane basis: plane containing the BH, camera, and ray direction
    vec3 e_r = normalize(rayOrigin);                        // BH -> camera
    vec3 n = normalize(cross(rayOrigin, rayDir));           // normal to orbital plane
    vec3 e_phi = cross(n, e_r);                             // tangent in orbital plane

    // Initial conditions
    float r0 = length(rayOrigin);
    float u0 = 1.0 / r0;

    // Decompose initial direction into radial and tangential components
    float dirRadial = dot(rayDir, e_r);
    float dirTangential = dot(rayDir, e_phi);

    // Impact parameter b = r * |tangential velocity|
    float b = r0 * abs(dirTangential);

    // Ray aimed almost straight at the BH -> captured
    if (b < 0.001 * rs) {
        return vec3(0.0);
    }

    // Initial du/dφ from energy conservation:
    // (du/dφ)² = 1/b² - u²(1 - 2Mu)
    float du2 = 1.0 / (b * b) - u0 * u0 * (1.0 - 2.0 * M * u0);
    float dup0 = 0.0;
    if (du2 > 0.0) {
        dup0 = dirRadial < 0.0 ? -sqrt(du2) : sqrt(du2);
    } else {
        // No valid orbit with this impact parameter -> captured
        return vec3(0.0);
    }

    // Integrate orbit equation forward
    float u = u0;
    float dup = dup0;
    float phi = 0.0;
    float dPhi = 0.01;
    int maxSteps = u_raySteps;
    float maxPhi = 10.0 * 3.141592653589793;  // Prevent infinite photon-sphere loops

    for (int i = 0; i < maxSteps; i++) {
        float r = 1.0 / u;

        // Captured by the BH
        if (r < horizon) {
            return vec3(0.0);
        }

        // Escaped to infinity
        if (r > u_maxDistance) {
            // Reconstruct look direction at escape from orbit state
            vec3 e_r_esc = cos(phi) * e_r + sin(phi) * e_phi;
            vec3 e_phi_esc = -sin(phi) * e_r + cos(phi) * e_phi;
            float alpha_esc = atan2(-u * dup, 1.0);
            int sign = dup >= 0.0 ? 1 : -1;
            vec3 lookDir = normalize(cos(alpha_esc) * e_r_esc + float(sign) * sin(alpha_esc) * e_phi_esc);
            return sampleStarField(lookDir);
        }

        // Prevent endless looping near photon sphere
        if (abs(phi) > maxPhi) {
            return sampleStarField(rayDir) * 0.5;
        }

        // RK4 step on orbit equation
        vec2 state = vec2(u, dup);
        state = orbitRK4Step(state, dPhi, M);
        u = state.x;
        dup = state.y;
        phi += dPhi;

        // Adaptive angular step: finer near photon sphere
        if (r < 6.0 * M) {
            dPhi = 0.002;
        } else if (r < 20.0 * M) {
            dPhi = 0.005;
        } else {
            dPhi = 0.01;
        }
    }

    // Step budget exhausted
    return sampleStarField(rayDir) * 0.5;
}

// Volumetric accretion disk: ray-march the disk volume instead of testing a
// single plane.  Density ~ r^(-3/2) with a Gaussian scale height, temperature
// ~ r^(-3/4) (Keplerian), blackbody emissivity ~ T^4, optical depth
// integration, and relativistic Doppler beaming from the azimuthal flow.
//
// Returns the SELF-ATTENUATED emission integral: each step's emissivity is
// multiplied by exp(-tau_so_far), i.e. the accumulated integral of
// j(s) * exp(-tau(s)) ds.  tau_out carries the total optical depth along the
// ray so the caller can composite the disk against the background with the
// correct radiative-transfer law:
//   color = color_bg * exp(-tau) + emission
// (additive emission alone is physically wrong: it makes an optically thick
// disk vanish and an optically thin disk stay bright.)
vec3 volumetricDiskEmission(vec3 rayOrigin, vec3 rayDir, float rs, float isco,
                           out float tauOut) {
    if (!u_volumetricDiskEnable) { tauOut = 0.0; return vec3(0.0); }

    float rInner = u_volumetricDiskInner;
    if (rInner <= 0.0) rInner = isco;
    float rOuter = u_volumetricDiskOuter;
    float steps = float(u_volumetricDiskSteps);
    float stepSize = u_maxDistance / steps;
    float opacity = u_volumetricDiskOpacity;
    float density = u_volumetricDiskDensity;
    float temp0 = u_volumetricDiskTemperature;
    float H0 = u_volumetricDiskScaleHeight;

    vec3 pos = rayOrigin;
    vec3 dir = normalize(rayDir);
    vec3 emissivity = vec3(0.0);
    float tau = 0.0;

    for (int i = 0; i < 256; i++) {
        if (i >= int(steps)) break;

        float r = length(pos);
        if (r < 0.001 || r > u_maxDistance) break;

        // Disk plane is y = 0; spin axis is +y
        float z = pos.y;
        float rPlane = length(pos.xz);

        if (rPlane > rOuter) continue;

        // Smooth transition at the inner edge so the ISCO is not a hard cut-off.
        // fadeWidth = 0 -> step function (legacy behaviour); >0 -> smoothstep over
        // the interval [rInner, rInner + fadeWidth].
        float innerFade = (u_volumetricDiskInnerEdgeFade > 0.0)
            ? smoothstep(rInner, rInner + u_volumetricDiskInnerEdgeFade, rPlane)
            : step(rInner, rPlane);

        // Scale height H/r = H0 (constant), H = H0 * r
        float H = H0 * rPlane;
        float zNorm = (H > 0.0001) ? z / H : 0.0;
        float rho = density * pow(rInner / rPlane, 1.5) * exp(-zNorm * zNorm);
        rho *= innerFade;

        // Keplerian temperature profile T ~ r^(-3/4)
        float T = temp0 * pow(rInner / rPlane, 0.75);
        // Doppler-shifted effective temperature.  The observed spectrum is a
        // blackbody at T_obs = T * delta, not a delta^4 intensity boost
        // applied to the emitted temperature.  The delta^4 factor moves into
        // j below; beaming carries only the user gain.
        float T_eff = T * delta;
        // Kirchhoff's law: j = κ·ρ·B(T_eff) ∝ κ·ρ·T_eff⁴.
        float j = opacity * rho * T_eff * T_eff * T_eff * T_eff;
        vec3 bbColor = blackbodyColor(T_eff);

        // Optical depth over this step
        float dTau = rho * opacity * stepSize;
        float T_before = exp(-tau);
        float aStep    = 1.0 - exp(-dTau);
        // Thin-slice series: (1-e^-dTau)/dTau → 1 - dTau/2 as dTau → 0.
        float avg      = (dTau > 1e-3) ? (aStep / dTau) : (1.0 - 0.5 * dTau);

        // Doppler beaming: azimuthal orbital velocity v_phi = sqrt(M/r)
        // (geometric units, c = 1). Prograde for Kerr.
        float vPhi = sqrt(u_mass / rPlane);
        float cosPhi = (rPlane > 0.0001) ? pos.x / rPlane : 0.0;
        float sinPhi = (rPlane > 0.0001) ? pos.z / rPlane : 0.0;
        // Velocity vector in the disk plane (tangential, +phi direction)
        vec3 vel = vec3(-sinPhi, 0.0, cosPhi) * vPhi;
        float vDotN = dot(vel, dir);
        // Doppler factor delta = 1 / (1 - v . n); blueshift when v.n < 0.
        // The delta^4 intensity boost is already embedded in j via T_eff = T*delta,
        // so beaming here is just the user gain.
        float delta = 1.0 / max(1.0 - vDotN, 0.01);
        float beaming = u_volumetricDiskDopplerBoost;

        // Self-attenuated contribution: T_before · j · ds · avg, tinted by
        // the Doppler-shifted blackbody color at T_obs = T * delta.
        emissivity += bbColor * T_before * j * stepSize * avg * beaming * u_volumetricDiskIntensity;
        tau += dTau;

        // Adaptive step near the black hole
        float adaptiveStep = stepSize * max(r / (3.0 * rs), 0.1);
        dir = normalize(dir + computeDeflection(pos, dir, u_mass, u_spin) * adaptiveStep);
        pos += dir * adaptiveStep;
    }

    tauOut = tau;
    return emissivity;
}

// Render photon ring glow
vec3 renderPhotonRing(float dist) {
    if (!u_enablePhotonRing) return vec3(0.0);

    float rs = 2.0 * u_mass;
    float photonR = 1.5 * rs * sqrt(3.0);  // Apparent photon ring radius

    float ringWidth = rs * 0.3;
    float ringDist = abs(dist - photonR);

    if (ringDist > ringWidth) return vec3(0.0);

    float intensity = (1.0 - ringDist / ringWidth);
    intensity = pow(intensity, 2.0);

    return vec3(0.6, 0.7, 1.0) * intensity * u_photonRingIntensity;
}

void main() {
    // Reconstruct ray direction from camera
    vec2 ndc = vTexCoord * 2.0 - 1.0;
    float aspect = float(textureSize(u_starField, 0).x) / float(textureSize(u_starField, 0).y);
    float tanHalfFov = tan(u_fov * 0.5);

    vec3 rayDir = normalize(
        u_cameraDir +
        u_cameraUp * ndc.y * tanHalfFov +
        normalize(cross(u_cameraDir, u_cameraUp)) * ndc.x * tanHalfFov * aspect
    );

    // Ray march through curved spacetime
    vec3 color = rayMarch(u_cameraPos, rayDir);

    // Volumetric accretion disk: integrate emissivity along the lensed ray.
    // ISCO from the exact Bardeen-Press-Teukolsky formula (matches the CPU
    // computeISCO()); the previous mix() approximation diverged badly for
    // high spin, which is exactly the visually dominant regime.
    float rs = 2.0 * u_mass;
    float isco = computeISCO(u_spin) * u_mass;
    float diskTau = 0.0;
    vec3 diskEmission = volumetricDiskEmission(u_cameraPos, rayDir, rs, isco, diskTau);

    // Radiative-transfer composite.  volumetricDiskEmission returns the
    // self-attenuated emission integral  ∫ j(s) exp(-tau(s)) ds, and diskTau
    // is the total optical depth to the observer.  The background (lensed
    // star field) is attenuated by exp(-tau) and the disk emission is added
    // on top -- this is the correct form in both limits:
    //   optically thin  (tau -> 0): background passes through, disk faint
    //   optically thick (tau -> infinity): background is occulted, disk
    //                                    saturates to its source function
    // The old code multiplied exp(-tau) INTO the emission, which made a thick
    // disk vanish and a thin disk stay bright -- both backwards.
    float transmission = exp(-diskTau);
    color = color * transmission + diskEmission;

    // Add photon ring glow
    float r = length(u_cameraPos);
    vec2 ndcPos = vTexCoord * 2.0 - 1.0;
    float distFromCenter = length(ndcPos) * r * 0.3;
    vec3 ringGlow = renderPhotonRing(distFromCenter);
    color += ringGlow;

    // Tone mapping
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}
