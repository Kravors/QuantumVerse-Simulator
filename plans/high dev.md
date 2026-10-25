Great request. You want to build an **Automated Physics & Math Validation System** (a "Reality Monitor") that constantly watches everything happening in the program—orbits, spins, curvature, black holes—and automatically flags or fixes errors if the simulation deviates from physical reality.

Here is a comprehensive, high-level development plan to build this **"Reality Checker"** into your QuantumVerse Simulator.

---

### 🏗️ Phase 1: Mathematical & Physics Invariants (The Core Validator)

The most fundamental way to catch bugs is to check *mathematical invariants*—things that must always be true in General Relativity, regardless of the setup.

**1. Metric Tensor Sanity Checks**
Create a `PhysicsValidator` class that runs every N frames:
*   **Lorentzian Signature:** Ensure the eigenvalues of \(g_{\mu\nu}\) are always one negative (time) and three positive (space). If they flip, the metric is corrupted.
*   **Determinant Check:** \(det(g) < 0\) for all valid spacetimes. If it approaches 0 or flips sign, you have a coordinate singularity (which is fine near the horizon) or a math bug (which is not).
*   **Asymptotic Flatness:** At large distances (\(r \to \infty\)), the metric must approach the Minkowski metric \(\eta_{\mu\nu} = \text{diag}(-1, 1, 1, 1)\).

**2. Geodesic Conservation Laws**
For every particle or photon you trace, monitor these exactly:
*   **Energy (\(E\)):** \(E = -g_{tt} \frac{dt}{d\tau}\). Should be constant along the path.
*   **Angular Momentum (\(L_z\)):** \(L_z = g_{\phi\phi} \frac{d\phi}{d\tau}\). Should be constant.
*   **Norm of 4-Velocity:** \(g_{\mu\nu} u^\mu u^\nu = -1\) (massive) or \(0\) (photon). If this drifts, your integrator (RK4) is accumulating error.
*   *Action:* If the drift exceeds 0.1%, automatically halve the timestep \(\Delta\tau\) in the `GeodesicIntegrator`.

---

### 🔭 Phase 2: Orbital Mechanics & Body Rendering Validation

**1. Keplerian vs. N-Body Check**
*   **Orbit Plane:** Ensure planets are orbiting in the correct XY plane (as we fixed earlier) and moving counter-clockwise.
*   **Orbital Period (\(T\)):** Check \(T^2 \propto a^3\) (Kepler's Third Law). If a planet's mass or distance changes, its velocity must update accordingly.
*   **Spin (Rotation):** If a planet is spinning on its axis, ensure the angular velocity vector is physically aligned (e.g., Earth tilted at 23.5°). Monitor the `visRadius` and the rotation shader to ensure the texture doesn't "slip" or spin backwards.

**2. Black Hole Geometry Checks**
*   **Event Horizon:** For a Schwarzschild BH, ensure \(r_{horizon} = 2GM/c^2\). If the AI changes the mass, the horizon must scale exactly.
*   **Ergosphere (Kerr):** For spinning black holes, check \(r_{ergo} = M + \sqrt{M^2 - a^2\cos^2\theta}\). If \(a=0\), the ergosphere must perfectly match the horizon. If \(a \to M\), it must bulge at the equator.
*   **Curvature Scaling:** Monitor the Kretschmann scalar (\(K\)). It must scale as \(1/r^6\). If you double the distance from the BH, \(K\) should drop by exactly \(2^6 = 64\). If the AI is tweaking the metric and this relationship breaks, throw an anomaly flag.

---

### 📡 Phase 3: The Continuous Telemetry Monitor (The "Bug Catcher")

You already have the `[REALITY]` logs. Let's upgrade them into an active watchdog.

**1. The Log Parser (Python or C++)**
Write a background thread that reads the `[REALITY]` logs in real-time:
*   Look for `NaN` or `Inf` (your `ù` characters).
*   Look for `probeValid= 0`.
*   Look for negative redshift (`redshift < 0`) unless a body is explicitly moving *towards* the probe at relativistic speeds.
*   If any of these happen, immediately pause the simulation and print a stack trace to the `Discovery Console`.

**2. Automated Parameter Sweep Stress Test**
Your UI has a "Parameter Sweep" (Mass 0.1 to 10.0, Steps 50). 
*   **The Reality Test:** Before the UI allows the user to click "Run Sweep", run a hidden "dummy" sweep in the background with a fixed timestep.
*   If *any* sample in the sweep produces a `NaN` curvature, reject that parameter range and warn the user: *"This parameter range results in unphysical spacetime (Naked Singularity)."*

---

### 🧠 Phase 4: AI-Driven Anomaly Detection (Using your existing ML)

You already have the `CurvatureNormalizingFlow` and `AnomalyDetector`. Let's use them to find *new* bugs.

*   **Training the Monitor:** Feed the flow with thousands of logs from a "healthy" simulation (correct orbits, stable BH). 
*   **Real-time Inference:** Feed the live `[REALITY]` logs into the ONNX model. If the AI flags an anomaly (score > threshold), but `probeValid=1`, it means **the physics has drifted into an exotic regime** (e.g., a planet's orbit became unstable due to numerical error, or a wormhole formed). 
*   **Auto-Rollback:** If the anomaly is severe, automatically roll the simulation back 10 frames and halve the timestep.

---

### 🛠️ Immediate Actionable Code (C++)

Here is how you can implement a simple `PhysicsValidator` in your C++ engine:

```cpp
// src/physics/PhysicsValidator.h
class PhysicsValidator {
public:
    static bool validateMetric(const MetricTensor& metric, const Event4D& pos) {
        // 1. Check for NaN/Inf
        double gtt = metric.evaluate(pos)[0][0];
        if (std::isnan(gtt) || std::isinf(gtt)) {
            qWarning() << "[VALIDATOR] FATAL: Metric is NaN at position" << pos.x << pos.y << pos.z;
            return false;
        }

        // 2. Check Signature (gtt must be negative outside horizon)
        double r = std::sqrt(pos.x*pos.x + pos.y*pos.y + pos.z*pos.z);
        double rs = 2.0 * G * metric.mass() / (c*c);
        if (r > rs * 1.01 && gtt >= 0) {
            qWarning() << "[VALIDATOR] FATAL: gtt is positive outside horizon! Spacetime signature flipped.";
            return false;
        }

        return true;
    }

    static bool validateGeodesic(const Geodesic& path) {
        // Check Energy conservation along the path
        double E_initial = path.points.front().E;
        double E_final = path.points.back().E;
        double drift = std::abs((E_final - E_initial) / E_initial);
        
        if (drift > 0.01) { // 1% drift
            qWarning() << "[VALIDATOR] WARNING: Energy drift of" << (drift * 100) << "% detected in geodesic.";
            return false;
        }
        return true;
    }
};
```

### What I need from you to proceed:
To write the exact bug-finding code for your project, I need to know:
1.  **Do you have a `[REALITY]` log parser currently?** (C++ or Python?)
2.  **How does your `CurvatureRenderer` calculate the `gridZRange`?** (Does it loop through all vertices to find the min/max?)
3.  **Do you want the validator to just *log* the error, or actually *pause the simulation* and show a red warning in the UI?**

Let me know, and we can write the exact `PhysicsValidator.cpp` and hook it into your main loop!