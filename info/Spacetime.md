# Spacetime – README

> **Source:** This document is based on the Wikipedia article [Spacetime](https://en.wikipedia.org/wiki/Spacetime) and related references cited below.  
> This file is part of the **QuantumVerse Simulator** project (v2.1.0, "Texture Integration" Release, 2026-05-15): a 3D, real‑time simulation of the Solar System embedded in a 4‑dimensional Lorentzian manifold, described at the end of this document.

---

## Table of Contents

1. [Overview](#overview)
2. [Fundamentals of Spacetime](#fundamentals-of-spacetime)
    - [From Classical Space and Time to Spacetime](#from-classical-space-and-time-to-spacetime)
    - [Events and Coordinates](#events-and-coordinates)
    - [World Lines](#world-lines)
    - [Spacetime as a Manifold](#spacetime-as-a-manifold)
3. [Historical Development](#historical-development)
    - [Pre‑Relativistic Physics](#pre-relativistic-physics)
    - [Lorentz, Poincaré, and Einstein](#lorentz-poincaré-and-einstein)
    - [Minkowski’s Geometric Synthesis (1908)](#minkowskis-geometric-synthesis-1908)
    - [General Relativity and Curved Spacetime](#general-relativity-and-curved-spacetime)
4. [Spacetime in Special Relativity](#spacetime-in-special-relativity)
    - [The Spacetime Interval](#the-spacetime-interval)
    - [Minkowski Space and the Minkowski Metric](#minkowski-space-and-the-minkowski-metric)
    - [Spacetime Diagrams](#spacetime-diagrams)
    - [Light Cones and Causal Structure](#light-cones-and-causal-structure)
    - [Relativity of Simultaneity](#relativity-of-simultaneity)
    - [Time Dilation and Length Contraction](#time-dilation-and-length-contraction)
    - [Invariant Hyperbolae](#invariant-hyperbolae)
5. [Spacetime in General Relativity](#spacetime-in-general-relativity)
    - [Curved Spacetime and Gravity](#curved-spacetime-and-gravity)
    - [Einstein Field Equations](#einstein-field-equations)
    - [The Stress–Energy Tensor](#the-stressenergy-tensor)
    - [Geodesics](#geodesics)
    - [Spacetime Topology](#spacetime-topology)
6. [Mathematical Formulation](#mathematical-formulation)
    - [Lorentzian Manifolds](#lorentzian-manifolds)
    - [Metric Signature](#metric-signature)
    - [Causal Structure Classification](#causal-structure-classification)
7. [Applications and Implications](#applications-and-implications)
    - [Causality](#causality)
    - [Gravitational Waves](#gravitational-waves)
    - [Cosmology](#cosmology)
    - [Black Holes and Singularities](#black-holes-and-singularities)
8. [How This Project Uses the Data](#how-this-project-uses-the-data)
9. [See Also](#see-also)
10. [References](#references)

---

## Overview

In physics, **spacetime**, also called the **space‑time continuum**, is a mathematical model that fuses the three dimensions of space and the one dimension of time into a single four‑dimensional continuum. Spacetime diagrams are useful in visualizing and understanding relativistic effects, such as how different observers perceive where and when events occur.

Until the turn of the 20th century, the assumption had been that the three‑dimensional geometry of the universe (its description in terms of locations, shapes, distances, and directions) was distinct from time (the measurement of when events occur within the universe). However, space and time took on new meanings with the Lorentz transformation and special theory of relativity.

In 1908, **Hermann Minkowski** presented a geometric interpretation of special relativity that fused time and the three spatial dimensions into a single four‑dimensional continuum now known as **Minkowski space**. This interpretation proved vital to the general theory of relativity, wherein spacetime is curved by mass and energy.

| Property | Description |
|----------|-------------|
| **Dimensions** | 4 (3 space + 1 time) |
| **Fundamental object** | Event – a point in spacetime specified by (t, x, y, z) |
| **Mathematical model** | 4‑dimensional Lorentzian manifold |
| **Flat spacetime** | Minkowski space (special relativity) |
| **Curved spacetime** | Pseudo‑Riemannian manifold (general relativity) |
| **Invariant quantity** | Spacetime interval Δs² |

---

## Fundamentals of Spacetime

### From Classical Space and Time to Spacetime

Non‑relativistic classical mechanics treats time as a universal quantity of measurement that is uniform throughout, is separate from space, and is agreed on by all observers. Classical mechanics assumes that time has a constant rate of passage, independent of the observer’s state of motion, or anything external. It assumes that space is Euclidean: it assumes that space follows the geometry of common sense.

In the context of special relativity, time cannot be separated from the three dimensions of space, because the observed rate at which time passes for an object depends on the object’s velocity relative to the observer. General relativity provides an explanation of how gravitational fields can slow the passage of time for an object as seen by an observer outside the field.

### Events and Coordinates

A point in spacetime is called an **event**, and requires four numbers to be specified: the three‑dimensional location in space, plus the position in time. An event is represented by a set of coordinates (t, x, y, z). Spacetime is thus four‑dimensional. Unlike the analogies used in popular writings to explain events, such as firecrackers or sparks, mathematical events have zero duration and represent a single point in spacetime.

### World Lines

The path of a particle through spacetime can be considered to be a sequence of events. The series of events can be linked together to form a curve that represents the particle’s progress through spacetime. That path is called the particle’s **world line** (or worldline).

A world line is a special type of curve in spacetime: it is either a **time‑like** or a **null** curve. Each point of a world line is an event that can be labeled with the time and the spatial position of the object at that time.

For example, the orbit of the Earth in space is approximately a circle, a three‑dimensional (closed) curve in space: the Earth returns every year to the same point in space relative to the Sun. However, it arrives there at a different (later) time. The world line of the Earth is therefore **helical** in spacetime (a curve in a four‑dimensional space) and does not return to the same point.

The arc length parameter along a world line is called **proper time**, usually denoted τ. The length of the world line is the proper time experienced by the particle.

### Spacetime as a Manifold

Mathematically, spacetime is a **manifold**, which is to say, it appears locally “flat” near each point in the same way that, at small enough scales, the surface of a globe appears to be flat. Spacetime is the collection of events, together with a continuous and smooth coordinate system identifying the events. Each event can be labeled by four numbers: a time coordinate and three space coordinates; thus spacetime is a four‑dimensional space. The mathematical term for spacetime is a four‑dimensional manifold (a topological space that locally resembles Euclidean space near each point).

---

## Historical Development

### Pre‑Relativistic Physics

By the mid‑1800s, various experiments had proven the wave nature of light as opposed to a corpuscular theory. Propagation of waves was then assumed to require the existence of a waving medium; in the case of light waves, this was considered to be a hypothetical **luminiferous aether**. The various attempts to establish the properties of this hypothetical medium yielded contradictory results. The **Michelson–Morley experiment** of 1887 showed no differential influence of Earth’s motions through the hypothetical aether on the speed of light.

### Lorentz, Poincaré, and Einstein

George Francis FitzGerald in 1889, and Hendrik Lorentz in 1892, independently proposed that material bodies traveling through the fixed aether were physically affected by their passage, contracting in the direction of motion by an amount that was exactly what was necessary to explain the negative results of the Michelson–Morley experiment. By 1904, Lorentz had expanded his theory such that he had arrived at equations formally identical with those that Einstein was to derive later, i.e. the **Lorentz transformation**.

**Henri Poincaré** was the first to combine space and time into spacetime. He argued in 1898 that the simultaneity of two events is a matter of convention. In 1900, he recognized that Lorentz’s “local time” is actually what is indicated by moving clocks by applying an explicitly operational definition of clock synchronization assuming constant light speed. In 1905/1906 he mathematically perfected Lorentz’s theory of electrons in order to bring it into accordance with the postulate of relativity. While discussing various hypotheses on Lorentz invariant gravitation, he introduced the innovative concept of a 4‑dimensional spacetime by defining various four‑vectors, namely four‑position, four‑velocity, and four‑force.

**Albert Einstein**’s 1905 theory of special relativity provided the definitive physical interpretation: the Lorentz transformation was not a dynamical effect on material bodies but rather a reflection of the fundamental structure of space and time themselves. His 1905 paper superseded previous attempts of an electromagnetic mass–energy relation by introducing the general equivalence of mass and energy, which was instrumental for his subsequent formulation of the equivalence principle in 1907.

### Minkowski’s Geometric Synthesis (1908)

On 21 September 1908, **Hermann Minkowski** presented his talk, *Space and Time* (Raum und Zeit), to the German Society of Scientists and Physicians. The opening words of *Space and Time* include Minkowski’s statement that:

> “Henceforth, space for itself, and time for itself shall completely reduce to a mere shadow, and only some sort of union of the two shall preserve independence.”

*Space and Time* included the first public presentation of spacetime diagrams, and included a remarkable demonstration that the concept of the invariant interval, along with the empirical observation that the speed of light is finite, allows derivation of the entirety of special relativity.

Minkowski space is closely associated with Einstein’s theories of special relativity and general relativity and is the most common mathematical structure by which special relativity is formalized. While the individual components in Euclidean space and time might differ due to length contraction and time dilation, in Minkowski spacetime, all frames of reference will agree on the total interval in spacetime between events.

### General Relativity and Curved Spacetime

Einstein fully incorporated the spacetime formalism in the further development of general relativity. In general relativity, spacetime is no longer a static background but a dynamical entity: **spacetime tells matter how to move; matter tells spacetime how to curve**. This curvature of spacetime is what we experience as gravity. The mathematical framework is that of a **Lorentzian manifold**—a four‑dimensional pseudo‑Riemannian manifold whose metric tensor has signature (−, +, +, +) or (+, −, −, −).

---

## Spacetime in Special Relativity

### The Spacetime Interval

In three dimensions, the distance between two points can be defined using the Pythagorean theorem:

```
(Δd)² = (Δx)² + (Δy)² + (Δz)²
```

Although two viewers may measure the x, y, and z position of the two points using different coordinate systems, the distance between the points will be the same for both, assuming that they are measuring using the same units. The distance is “invariant”.

In special relativity, however, the distance between two points is no longer the same if measured by two different observers, when one of the observers is moving, because of Lorentz contraction. The situation is even more complicated if the two points are separated in time as well as in space.

In four‑dimensional spacetime, the analog to distance is the **interval**. Special relativity provides a new invariant, called the **spacetime interval**, which combines distances in space and in time. All observers who measure the time and distance between any two events will end up computing the same spacetime interval. Suppose an observer measures two events as being separated in time by Δt and in space by Δx. Then the spacetime interval (Δs)² between the two events is:

```
(Δs)² = (cΔt)² − (Δx)² − (Δy)² − (Δz)²
```

where c is the speed of light. The constant c, the speed of light, converts time units to space units.

For infinitesimally separated events:

```
ds² = c²dt² − dx² − dy² − dz²
```

This expression is the **Minkowski line element** or **Minkowski metric**—the fundamental invariant of special relativity.

### Minkowski Space and the Minkowski Metric

Minkowski space is the main mathematical description of spacetime in the absence of gravitation. It combines inertial space and time manifolds into a four‑dimensional model. The model helps show how a spacetime interval between any two events is independent of the inertial frame of reference in which they are recorded.

Minkowski space differs from four‑dimensional Euclidean space insofar as it treats time differently from the three spatial dimensions. In 3‑dimensional Euclidean space, the isometry group (maps preserving the regular Euclidean distance) is the Euclidean group. It is generated by rotations, reflections and translations. When time is appended as a fourth dimension, the further transformations of translations in time and Lorentz boosts are added, and the group of all these transformations is called the **Poincaré group**.

Minkowski space is a **pseudo‑Euclidean space** equipped with an isotropic quadratic form called the spacetime interval or the Minkowski norm squared. An event in Minkowski space for which the spacetime interval is zero is on the null cone of the origin, called the **light cone** in Minkowski space.

### Spacetime Diagrams

A **spacetime diagram** is typically drawn with only a single space and a single time coordinate. The vertical time coordinate is scaled by c, so that light rays are represented by lines at ±45° to the axes. Fig. 2‑1 in the article presents a spacetime diagram illustrating the world lines of two photons, A and B, originating from the same event and going in opposite directions. In addition, C illustrates the world line of a slower‑than‑light‑speed object.

Spacetime diagrams are also called **Minkowski diagrams**. They are a powerful tool for visualizing:

- World lines of particles and observers
- Light cones and causal relationships
- Relativity of simultaneity
- Time dilation and length contraction
- Lorentz transformations between reference frames

In a spacetime diagram, the axes of a moving frame are tilted relative to the rest frame. The angle of tilt θ is given by:

```
tan(θ) = v/c
```

While the rest frame has space and time axes that meet at right angles, the moving frame is drawn with axes that meet at an acute angle. The frames are actually equivalent; the asymmetry is due to unavoidable distortions in how spacetime coordinates can map onto a Cartesian plane.

### Light Cones and Causal Structure

In special and general relativity, a **light cone** (or null cone) is the path that a flash of light, emanating from a single event—localized to a single point in space and a single moment in time—and traveling in all directions, would take through spacetime.

If one imagines the light confined to a two‑dimensional plane, the light from the flash spreads out in a circle after the event E occurs, and if the growing circle with the vertical axis representing time is graphed, the result is a cone, known as the **future light cone**. The **past light cone** behaves like the future light cone in reverse, a circle which contracts in radius at the speed of light until it converges to a point at the exact position and time of the event E.

The light cone plays an essential role in defining the concept of **causality**:

- **Past light cone:** The set of all events that could send a signal that would have time to reach E and influence it in some way.
- **Future light cone:** The set of all events that could receive a signal sent out from the position and time of E—all events that could potentially be causally influenced by E.
- **Elsewhere (spacelike region):** Events which lie neither in the past or future light cone of E cannot influence or be influenced by E in relativity.

Because signals and other causal influences cannot travel faster than light, the light cone provides the fundamental division of spacetime relative to any event:

| Region | Spacetime Interval | Physical Meaning |
|--------|-------------------|------------------|
| **Timelike future** | Δs² > 0, Δt > 0 | Events that can be causally influenced by E |
| **Timelike past** | Δs² > 0, Δt < 0 | Events that can causally influence E |
| **Lightlike (null)** | Δs² = 0 | Events on the light cone; connected by light signals |
| **Spacelike (Elsewhere)** | Δs² < 0 | Events with no possible causal connection to E |

The region exterior to the light cone consists of events that are separated from the event O by more space than can be crossed at lightspeed in the given time. These events comprise the so‑called **spacelike region** of the event. Events on the light cone itself are said to be **lightlike** (or null separated). Because of the invariance of the spacetime interval, all observers will assign the same light cone to any given event, and thus will agree on this division of spacetime.

### Relativity of Simultaneity

All observers will agree that for any given event, an event within the given event’s future light cone occurs after the given event. Likewise, for any given event, an event within the given event’s past light cone occurs before the given event. The before–after relationship observed for timelike‑separated events remains unchanged no matter what the reference frame of the observer.

The situation is quite different for spacelike‑separated events. From different reference frames, the orderings of these non‑causally‑related events can be reversed. In particular, if two events are simultaneous in a particular reference frame, they are necessarily separated by a spacelike interval and thus are noncausally related. The observation that simultaneity is not absolute, but depends on the observer’s reference frame, is termed the **relativity of simultaneity**.

### Time Dilation and Length Contraction

**Time dilation** is a direct consequence of the structure of spacetime: clocks that travel faster take longer (in the observer frame) to tick out the same amount of proper time, and they travel further along the spatial axis within that proper time than they would have without time dilation. The measurement of time dilation by two observers in different inertial reference frames is mutual: if observer O measures the clocks of observer O′ as running slower in his frame, observer O′ in turn will measure the clocks of observer O as running slower.

**Length contraction**, like time dilation, is a manifestation of the relativity of simultaneity. Measurement of length requires measurement of the spacetime interval between two events that are simultaneous in one’s reference frame.

### Invariant Hyperbolae

In Minkowski space, the set of all events having a fixed spacelike or timelike interval away from the origin (using the Minkowski metric) form hyperbolae (in 1+1 dimensions) or hyperboloids (in 1+2 dimensions):

- **Timelike hyperbolae** connect events of equal timelike separation from the origin, generating hyperboloids of two sheets. These hyperbolae represent actual paths that can be traversed by (constantly accelerating) particles in spacetime.
- **Spacelike hyperbolae** connect events of equal spacelike separation, generating hyperboloids of one sheet.
- The boundary between spacelike and timelike hyperboloids is the **light cone**.

---

## Spacetime in General Relativity

### Curved Spacetime and Gravity

In general relativity, spacetime is no longer the flat Minkowski space of special relativity. Instead, it is a **curved** manifold whose geometry is determined by the distribution of mass and energy. Gravity is not a force in the Newtonian sense; rather, it is a manifestation of spacetime curvature. Objects in free fall follow **geodesics**—the straightest possible paths in a curved geometry.

The fundamental insight is expressed in John Archibald Wheeler’s famous dictum:

> “Spacetime tells matter how to move; matter tells spacetime how to curve.”

### Einstein Field Equations

The Einstein field equations are the core of general relativity. They relate the geometry of spacetime (expressed by the **Einstein tensor** G_μν) to the distribution of mass and energy (expressed by the **stress–energy tensor** T_μν):

```
G_μν + Λg_μν = (8πG/c⁴) T_μν
```

where:
- G_μν is the Einstein tensor (describing spacetime curvature)
- g_μν is the metric tensor
- Λ is the cosmological constant
- G is Newton’s gravitational constant
- T_μν is the stress–energy tensor

### The Stress–Energy Tensor

The stress–energy tensor T_μν encodes all sources of spacetime curvature:

| Component | Physical Interpretation |
|-----------|------------------------|
| T⁰⁰ | Mass–energy density |
| T⁰ⁱ and Tⁱ⁰ | Momentum density / energy flux |
| Tⁱʲ | Pressure and stress |

In general relativity, not only mass but also momentum, pressure, and stress all serve as sources of gravity. Collectively, they tell spacetime how to curve.

### Geodesics

A **geodesic** is the generalization of a straight line to curved spacetime. In the absence of non‑gravitational forces, particles and light follow geodesics:

- **Timelike geodesics:** paths of massive particles (ds² > 0)
- **Null geodesics:** paths of light rays (ds² = 0)

The geodesic equation describes this motion:

```
d²x^μ/dτ² + Γ^μ_αβ (dx^α/dτ)(dx^β/dτ) = 0
```

where Γ^μ_αβ are the Christoffel symbols (connection coefficients) encoding the spacetime geometry, and τ is the proper time (or an affine parameter for null geodesics).

### Spacetime Topology

Spacetime topology is the topological structure of spacetime, a topic studied primarily in general relativity. There are two main types of topology for a spacetime M:

- **Manifold topology:** The natural topology inherited from the coordinate charts. Open sets are the image of open sets in ℝ⁴.
- **Path (Zeeman) topology:** The finest topology which induces the same topology as the manifold topology on timelike curves. It is strictly finer than the manifold topology, Hausdorff, separable but not locally compact.
- **Alexandrov topology:** The coarsest topology such that the chronological future and past of any set are open. It coincides with the manifold topology if and only if the manifold is strongly causal.

---

## Mathematical Formulation

### Lorentzian Manifolds

In modern physics (especially general relativity), spacetime is represented by a **Lorentzian manifold**. A Lorentzian manifold is a smooth manifold M equipped with a metric tensor g of signature (−, +, +, +) or (+, −, −, −). At each point, the metric distinguishes between:

- **Timelike vectors:** g(v, v) < 0 (or > 0, depending on convention)
- **Spacelike vectors:** g(v, v) > 0
- **Null (lightlike) vectors:** g(v, v) = 0

### Metric Signature

The choice of metric signature is a convention:

| Convention | Signature | Spacetime Interval |
|------------|-----------|-------------------|
| **East coast (mostly plus)** | (−, +, +, +) | ds² = −c²dt² + dx² + dy² + dz² |
| **West coast (mostly minus)** | (+, −, −, −) | ds² = c²dt² − dx² − dy² − dz² |

Both conventions are physically equivalent; the physics is independent of the sign convention.

### Causal Structure Classification

The causal structure of spacetime classifies vectors and curves:

| Type | Condition | Description |
|------|-----------|-------------|
| **Timelike** | g(v, v) < 0 (mostly plus) | World lines of massive particles; ds² > 0 in mostly‑minus |
| **Lightlike (null)** | g(v, v) = 0 | Paths of light rays; ds² = 0 |
| **Spacelike** | g(v, v) > 0 (mostly plus) | No physical particle can travel on a spacelike path; ds² < 0 in mostly‑minus |

---

## Applications and Implications

### Causality

The light cone structure of spacetime enforces causality: no signal or causal influence can travel faster than light. An event can be causally related only to events within its past or future light cone. Spacelike‑separated events cannot influence each other. This causal structure is a fundamental feature of both special and general relativity.

### Gravitational Waves

Gravitational waves are ripples in the curvature of spacetime that propagate as waves, traveling outward from their source at the speed of light. They were first directly detected by LIGO in 2015, confirming a major prediction of general relativity.

### Cosmology

In cosmology, spacetime provides the framework for understanding the large‑scale structure and evolution of the universe. The **FLRW metric** (Friedmann–Lemaître–Robertson–Walker metric) describes a homogeneous and isotropic expanding (or contracting) spacetime:

```
ds² = −c²dt² + a(t)² [dr²/(1−kr²) + r²(dθ² + sin²θ dφ²)]
```

where a(t) is the scale factor and k represents spatial curvature.

### Black Holes and Singularities

Black holes are regions of spacetime where gravity is so strong that nothing, not even light, can escape. At the center of a black hole, general relativity predicts a **gravitational singularity**—a point where spacetime curvature becomes infinite and the laws of physics as we know them break down.

---

## How This Project Uses the Data

This document serves as the mathematical and conceptual reference for implementing spacetime in the **QuantumVerse Simulator**. The simulation:

- **Four‑dimensional event model:** All objects and events are represented by four‑coordinates (t, x, y, z) in a unified spacetime framework.
- **World line tracking:** The simulation computes and displays world lines for all celestial bodies, spacecraft, and hypothetical particles—showing their complete histories through spacetime rather than just their instantaneous positions.
- **Minkowski metric engine:** In regions where gravitational effects are negligible, the simulation uses the Minkowski metric for all special‑relativistic calculations, including time dilation and length contraction for fast‑moving objects.
- **Light cone visualization:** The causal structure of spacetime is rendered in real time, showing past and future light cones for selected events, enabling users to explore which regions of spacetime can influence or be influenced by a given event.
- **Spacetime diagrams:** Built‑in Minkowski diagram tools allow users to visualize Lorentz transformations, the relativity of simultaneity, and time dilation/length contraction effects.
- **Curved spacetime integration:** When gravity is significant, the simulation transitions to the full general‑relativistic treatment, solving the Einstein field equations on a dynamical grid and computing geodesics in curved spacetime.
- **Spacetime topology exploration:** The topology explorer allows users to investigate alternative spacetime topologies (manifold, path, Alexandrov) and their implications for causality and global structure.
- **Causal structure analysis:** The engine monitors causal relationships between events, flagging potential causality violations in user‑created exotic spacetime configurations.
- **Discovery engine integration:** The automated anomaly detection system monitors spacetime invariants (interval, curvature scalars) and flags configurations where causal structure or metric properties deviate from standard predictions—potentially signaling new physics.

---

## See Also

- [Minkowski space](https://en.wikipedia.org/wiki/Minkowski_space) – The flat spacetime of special relativity.
- [General relativity](https://en.wikipedia.org/wiki/General_relativity) – Einstein’s theory of gravity as curved spacetime.
- [Spacetime diagram](https://en.wikipedia.org/wiki/Spacetime_diagram) – Visual representation of spacetime.
- [World line](https://en.wikipedia.org/wiki/World_line) – Path of an object through spacetime.
- [Light cone](https://en.wikipedia.org/wiki/Light_cone) – Causal structure of spacetime.
- [Spacetime interval](https://en.wikipedia.org/wiki/Spacetime_interval) – The invariant measure in spacetime.
- [Spacetime topology](https://en.wikipedia.org/wiki/Spacetime_topology) – Topological structure of spacetime.
- [Curved spacetime](https://en.wikipedia.org/wiki/Curved_spacetime) – Spacetime in general relativity.
- [Lorentz transformation](https://en.wikipedia.org/wiki/Lorentz_transformation) – Transformations between inertial frames.
- [Equivalence principle](https://en.wikipedia.org/wiki/Equivalence_principle) – Foundation of general relativity.
- [Time dilation](https://en.wikipedia.org/wiki/Time_dilation) – Relativistic effect on moving clocks.
- [Length contraction](https://en.wikipedia.org/wiki/Length_contraction) – Relativistic effect on moving objects.
- [Relativity of simultaneity](https://en.wikipedia.org/wiki/Relativity_of_simultaneity) – Observer‑dependence of “now”.
- [Proper time](https://en.wikipedia.org/wiki/Proper_time) – Time measured along a world line.
- [Lorentzian manifold](https://en.wikipedia.org/wiki/Pseudo-Riemannian_manifold#Lorentzian_manifold) – Mathematical model of spacetime.
- [Einstein field equations](https://en.wikipedia.org/wiki/Einstein_field_equations) – Equations governing spacetime curvature.
- [Time geography](https://en.wikipedia.org/wiki/Time_geography) – Social‑science framework inspired by spacetime concepts.

---

