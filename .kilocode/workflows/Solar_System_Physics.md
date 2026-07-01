[file name]: Solar_System_Physics.md
[file content begin]
# Solar_System_Physics.md

A comprehensive, self-contained reference to the physics of the Solar System.  
The document is structured as a progressive journey: from fundamental orbital mechanics and the Sun to planetary environments, small body dynamics, relativistic effects, and formation scenarios.  
Use the **Steps** below as a roadmap; each section can be read independently or in sequence.

## Steps

1. **Gravitational Dynamics & Orbital Mechanics** – Newton’s law, two‑body problem, N‑body perturbations, resonances, tidal forces and Yarkovsky/YORP effects.  
2. **The Sun** – Internal structure, nuclear fusion, atmospheric layers, magnetic activity, solar wind and heliosphere.  
3. **Planetary Atmospheres** – Hydrostatic equilibrium, radiative transfer, greenhouse effect, dynamics, and escape processes.  
4. **Planetary Interiors and Surfaces** – Equations of state, heat sources, convection, magnetic dynamos.  
5. **Small Bodies: Asteroids, Comets, Kuiper Belt** – Orbital distributions, collisions, cometary activity, and non‑gravitational forces.  
6. **Planetary Rings and Satellite Systems** – Roche limit, shepherd moons, density and bending waves.  
7. **Relativistic Effects** – Perihelion precession, Shapiro delay, frame‑dragging.  
8. **Formation and Evolution of the Solar System** – Protoplanetary disks, accretion, planet migration, and late dynamical instabilities.

---

## 1. Gravitational Dynamics & Orbital Mechanics

### 1.1 Newtonian Gravity and the Two-Body Problem

Newton’s law:  
\[ \vec{F}_{12} = -\,\frac{G m_1 m_2}{r^3}\,\vec{r}_{12} \]

For two bodies, relative motion satisfies:  
\[ \ddot{\vec{r}} = -\frac{\mu}{r^3}\vec{r}, \qquad \mu = G(m_1+m_2) \]

Kepler’s laws follow: conic sections, constant areal velocity, and \(P^2 \propto a^3\).

### 1.2 Orbital Elements and the Vis‑Viva Equation

Classical elements: \(a\) (semi‑major axis), \(e\) (eccentricity), \(i\) (inclination), \(\Omega\), \(\omega\), \(\nu\).  
Specific energy and angular momentum:

\[
\mathcal{E} = \frac{v^2}{2} - \frac{\mu}{r} = -\frac{\mu}{2a}, \qquad
\vec{h} = \vec{r} \times \vec{v}
\]

**Vis‑viva**:  
\[ v = \sqrt{\mu\left(\frac{2}{r} - \frac{1}{a}\right)} \]

### 1.3 The N‑Body Problem and Perturbations

N‑body equations:

\[
\ddot{\vec{r}}_i = -G\sum_{j\neq i}\frac{m_j}{r_{ij}^3}(\vec{r}_i - \vec{r}_j)
\]

No analytic solutions for \(N\ge3\). Studies rely on symplectic integrators and perturbation theory (disturbing function \(\mathcal{R}\)). Secular perturbation terms govern long‑term evolution of \(e\), \(i\), \(\varpi\), \(\Omega\). Important perturbations include planetary oblateness (\(J_2\)), third‑body effects, and relativistic corrections.

### 1.4 Lagrange Points and the CR3BP

In the Circular Restricted Three‑Body Problem, five equilibrium points exist: \(L_1\), \(L_2\), \(L_3\) (collinear, unstable), \(L_4\), \(L_5\) (triangular, stable for \(\mu \lesssim 0.0385\)).  
Equations in a rotating frame:

\[
\ddot{x} - 2\Omega\dot{y} = \frac{\partial U}{\partial x},\quad
\ddot{y} + 2\Omega\dot{x} = \frac{\partial U}{\partial y},\quad
\ddot{z} = \frac{\partial U}{\partial z}
\]

with \(U = \frac{1}{2}\Omega^2(x^2+y^2) + \frac{Gm_1}{r_1} + \frac{Gm_2}{r_2}\).

### 1.5 Orbital Resonances

**Mean‑motion resonance** (MMR): period ratio \(P_2/P_1 \approx p/(p+q)\). Resonant argument \(\phi = (p+q)\lambda_2 - p\lambda_1 - \varpi_i\) librates. Examples: Pluto–Neptune 3:2, Galilean moons Laplace resonance.  
**Secular resonances**: precession frequencies match a perturbing planet’s frequency, pumping eccentricities/inclinations (e.g., ν₆ resonance in the asteroid belt).

### 1.6 Tidal Forces, Locking, and Heating

Tidal potential:  
\[ \Phi_{\text{tide}} \approx -\frac{GM}{d^3} r^2 P_2(\cos\theta) \]

Torque lengthens the spin‑orbit locking timescale:

\[
t_{\text{lock}} \sim \frac{\omega a^6 m Q}{3 G M^2 k_2 R^5}
\]

Tidal heating rate (synchronous satellite, eccentricity \(e\)):

\[
\dot{E} \sim \frac{21}{2} \frac{k_2}{Q} \frac{G M^2 R^5 n e^2}{a^6}
\]

### 1.7 Precession, Nutation, and Radiation Forces

- Axial precession: Solar and lunar torques on Earth’s equator → ~26 000 yr.  
- Apsidal precession: planetary perturbations + GR (43″/century for Mercury).  
- **Yarkovsky effect**: anisotropic thermal emission alters semimajor axis; diurnal term \(\propto\cos\gamma\), seasonal \(\propto\sin\gamma\).  
- **YORP effect**: asymmetric thermal radiation changes rotation rate and obliquity.

---

## 2. The Sun

### 2.1 Stellar Structure and Nuclear Fusion

Hydrostatic equilibrium:

\[
\frac{dP}{dr} = -\frac{G m(r) \rho}{r^2},\qquad
\frac{dm}{dr} = 4\pi r^2 \rho
\]

Energy transport: radiative (inner) and convective (outer ~30%). Radiative gradient:

\[
\frac{dT}{dr} = -\frac{3\kappa\rho L}{16\pi a c r^2 T^3}
\]

Nuclear fusion: pp‑chain dominates (\(4p \rightarrow {}^4\text{He} + 26.73\ \text{MeV}\)), CNO cycle contributes ~1%. Energy generation rate \(\epsilon \propto \rho T^\nu\).

### 2.2 Solar Atmosphere and Magnetic Activity

- Photosphere (\(\tau_{5000}\sim1\)): \(T_{\text{eff}} = 5772\ \text{K}\), granulation, sunspots.  
- Chromosphere → transition region → corona (1–3 MK).  
- Solar dynamo: differential rotation (\(\omega\)-effect) + helical convection (\(\alpha\)-effect) regenerates the poloidal field. 11‑yr sunspot cycle, 22‑yr Hale magnetic cycle.  
- Flares and CMEs: magnetic reconnection release up to \(10^{25}\ \text{J}\).

### 2.3 Solar Wind and Heliosphere

Parker wind momentum equation (isothermal):

\[
\left(v - \frac{c_s^2}{v}\right)\frac{dv}{dr} = \frac{2c_s^2}{r} - \frac{GM_\odot}{r^2}
\]

Solution passes through a sonic point. Slow wind ~400 km/s, fast wind ~800 km/s. Heliosphere boundary: termination shock (~90 AU), heliopause, bow shock. Parker spiral IMF.

---

## 3. Planetary Atmospheres

### 3.1 Hydrostatic Equilibrium and Scale Height

\[
\frac{dP}{dz} = -\rho g \;\Rightarrow\; P(z) = P_0 e^{-z/H},\quad
H = \frac{k_B T}{\bar{m} g}
\]

Earth \(H \approx 8\ \text{km}\), Titan ~20 km, Mars ~12 km.

### 3.2 Radiative Transfer and Greenhouse Effect

Radiative transfer:  
\[ \mu \frac{dI_\nu}{d\tau_\nu} = I_\nu - S_\nu \]

Effective temperature:  
\[ T_e = \left[\frac{F_\odot(1-A)}{4\sigma}\right]^{1/4} \]

Earth \(T_e \sim 255\ \text{K}\); greenhouse gases (H₂O, CO₂, CH₄) raise surface to ~288 K.

### 3.3 Atmospheric Dynamics

Rotating frame momentum equation:

\[
\frac{D\vec{v}}{Dt} + 2\vec{\Omega}\times\vec{v} = -\frac{1}{\rho}\nabla p + \vec{g} + \vec{F}_{\text{fr}}
\]

Geostrophic balance: Coriolis force equals pressure gradient. Thermal wind relates vertical shear to horizontal temperature gradients. Zonal jets, Hadley circulation, Rossby waves.

### 3.4 Atmospheric Escape

- **Jeans escape**: flux \(\propto (1+X)e^{-X}\) with \(X = v_{\text{esc}}^2/v_{\text{th}}^2\).  
- **Hydrodynamic escape**: XUV‑driven, drags heavy species.  
- **Sputtering**, **photochemical escape**.

---

## 4. Planetary Interiors and Surfaces

### 4.1 Hydrostatic Equilibrium and Equations of State

\[
\frac{dP}{dr} = -\rho g,\quad g(r) = \frac{G m(r)}{r^2}
\]

EOS: Birch‑Murnaghan for solids, hydrogen‑helium EOS with insulator‑metal transition at ~1–3 Mbar; polytropes \(P = K\rho^{1+1/n}\) as approximations.

### 4.2 Heat Sources and Convection

- Accretional heat: \(\Delta T \sim \frac{GM}{C_P R}\).  
- Radiogenic: \(^{238}\text{U}, ^{235}\text{U}, ^{232}\text{Th}, ^{40}\text{K}\).  
- Tidal dissipation (see §1.6).  
- Core solidification latent heat.  
Convection at high Rayleigh number; plate tectonics on Earth, stagnant lid elsewhere.

### 4.3 Planetary Magnetism and Dynamos

Induction equation:

\[
\frac{\partial \vec{B}}{\partial t} = \nabla \times (\vec{v} \times \vec{B}) + \eta \nabla^2 \vec{B}
\]

Dynamo requires conducting fluid, convection, and rapid rotation (\(Rm = v L / \eta \gg 1\)). Scaling law: \(B \propto \rho^{1/2} (\Omega R D)^{1/3}\).

---

## 5. Small Bodies: Asteroids, Comets, Kuiper Belt

### 5.1 Orbital Distributions and Collisions

Asteroid belt Kirkwood gaps from Jupiter MMRs; Kuiper belt resonances with Neptune (3:2 plutinos, 2:1 twotinos). Collisional cascade: size distribution \(N(>D) \propto D^{-q}\), \(q \approx 2.5\).

### 5.2 Cometary Activity and Non‑Gravitational Forces

Sublimation energy balance:

\[
\frac{L\,dm}{dt} = \frac{F_\odot(1-A)}{r^2} \pi R_n^2 - 4\pi R_n^2 \epsilon\sigma T^4
\]

Radiation pressure parameter \(\beta = F_{\text{rad}}/F_{\text{grav}}\) shapes dust tails. Non‑gravitational acceleration (Marsden model) expressed as \(\Delta \ddot{\vec{r}} = (A_1 \hat{r} + A_2 \hat{t} + A_3 \hat{n})\,g(r)\).

### 5.3 Yarkovsky and YORP Effects

Diurnal Yarkovsky semimajor axis drift:  
\[ \frac{da}{dt} \propto \frac{\cos\gamma}{a^2} \]  
Seasonal variant \(\propto \sin\gamma\).  
YORP spin‑rate change:  
\[ \dot{\omega} \propto \frac{1}{a^2 \sqrt{1-e^2}} \]  
Can spin‑up to rotational disruption, forming binaries.

---

## 6. Planetary Rings and Satellite Systems

### 6.1 Roche Limit and Tidal Disruption

For a strengthless satellite:  
\[ d_{\text{Roche}} = R_p\left(2\frac{\rho_p}{\rho_s}\right)^{1/3} \]

Material inside this limit forms rings.

### 6.2 Ring Dynamics and Resonances

Keplerian shear, spiral density waves launched at Lindblad resonances (\(m(\Omega - \Omega_p) = \pm\kappa\)). Bending waves at vertical resonances. Shepherd moons confine narrow rings (e.g., Saturn’s F ring) via angular momentum transfer.

---

## 7. Relativistic Effects

Schwarzschild effective potential:  
\[ V_{\text{eff}} = -\frac{GM}{r} + \frac{L^2}{2r^2} - \frac{GM L^2}{c^2 r^3} \]

Per orbit precession:  
\[ \Delta \varpi = \frac{6\pi GM}{a(1-e^2)c^2} \]  
(Mercury: 42.98″/century). Also: Shapiro delay, light deflection (1.75″ at solar limb), Lense‑Thirring frame‑dragging.

---

## 8. Formation and Evolution of the Solar System

### 8.1 Nebular Hypothesis and Disk Physics

Gravitational collapse → protostar + protoplanetary disk. Minimum mass solar nebula:  
\[ \Sigma(r) = 1700\ (r/1\,\text{AU})^{-3/2}\ \text{kg m}^{-2} \]  
Temperature \(T \propto r^{-1/2}\), viscous α‑disk. Snow line at ~170 K separates terrestrial and giant planet regions.

### 8.2 Planetesimal Accretion

Dust → planetesimals (streaming instability, pebble accretion). Oligarchic growth: \(\dot{M} \propto \Sigma \Omega R_H^2\). Final terrestrial planets built by giant impacts (e.g., Earth–Moon system).

### 8.3 Giant Planet Formation and Migration

**Core accretion**: ~10 M🜨 core triggers rapid gas accretion.  
**Type I migration** (low mass): Lindblad/corotation torques, \(\tau \propto 1/M\).  
**Type II migration** (gap‑opening giants): locked to disk’s viscous evolution. Scenarios: Grand Tack (Jupiter migrates in, then out) and Nice model (late instability reshaping outer system).

### 8.4 Late Dynamical Instabilities

Secular chaos and planetary orbital evolution; Lyapunov time ~5 Myr. Current configuration stable on Gyr timescales but marginally so. Explains Late Heavy Bombardment and Kuiper belt structure.

---

*This reference is a living document. Contributions and corrections are welcome – please open an issue or pull request.*
[file content end]