# Upgraded Grill-Me Skill

**File**: `.kilo/skills/grill-me/SKILL.md`  
**Upgrade Version**: 2.0.0 (“Systematic Design Interrogation & Physics‑Grade Stress Testing”)  
**Date**: 2026-05-28  

This upgrade transforms the original “grill me” meta‑skill into a **structured, rigorous design interrogation system** that systematically walks through decision trees, verifies assumptions against the codebase (QuantumVerse), evaluates trade‑offs, flags risks, and produces a shared understanding. It supports **physics‑specific probing** (e.g., dimensional analysis, conservation laws, validation benchmarks) and integrates with the project’s existing modules (spacetime, physics, discovery, ML, etc.).

---

## Name
**grill-me**

## Description
Interview the user relentlessly about every aspect of a plan, design, or feature until reaching shared understanding. Walk down each branch of the decision tree, resolving dependencies between decisions one‑by‑one. For each question, provide your recommended answer based on best practices, known physics constraints, and QuantumVerse architecture. Use when the user wants to stress‑test a plan, get grilled on their design, or mentions “grill me”, “cross‑examine”, “challenge my design”, or “Socratic review”.

## Metadata
```yaml
project: QuantumVerse
version: 2.0.0
author: DHIAEDDINE0223
category: development | design | review
source:
  original: https://github.com/mattpocock/skills (grill-me)
  extended: QuantumVerse physics & architecture guidelines
dependencies:
  - QuantumVerse codebase (for code exploration)
  - validate-physics skill (optional, for quantitative checks)
```

---

## When to Use

Trigger this skill when the user:
- Says “grill me”, “cross‑examine my design”, “challenge me”, “poke holes in this plan”
- Mentions a new feature, algorithm, or architecture and wants rigorous feedback
- Asks “is this design sound?” or “what am I missing?”
- Presents a decision tree or design document for review

The skill **does not** trigger on general code reviews or bug fixes.

---

## Core Principles

1. **One question at a time** – never multi‑plex.
2. **Recommend an answer** after each question (based on evidence, precedent, physics, or code exploration).
3. **Explore the codebase** to answer any question that can be resolved by reading existing source (QuantumVerse modules, tests, docs).
4. **Force explicit trade‑offs** – for every decision, demand clarification of what is gained vs. lost.
5. **Demand validation** – how would you test that? What benchmark would confirm correctness?
6. **Physics‑specific probes** – for relativistic/quantum gravity features, ask about dimensional analysis, coordinate invariance, conservation laws, causal structure, numerical stability, etc.
7. **Risk enumeration** – at the end of each major branch, ask “what could go wrong?” and propose mitigations.

---

## Process

### Step 0: Understand the scope

Ask the user to briefly state the **plan, feature, or design** being reviewed. If the user provides a document or file path, read it (if accessible) or ask them to paste the relevant excerpt.

Example: *“What is the central idea you want me to grill you on? (e.g., ‘new geodesic integrator’, ‘dilaton black hole renderer’, ‘AI theory proposer’)”*

### Step 1: Systematic decision‑tree traversal

The skill uses a **dynamic decision tree template** that adapts to the domain. For physics‑related designs, the tree includes:

1. **Mathematical foundations** – what equations? what assumptions? what coordinate choices?
2. **Numerical methods** – discretization, time stepping, error control, stability.
3. **Software architecture** – module placement, dependencies, API design, thread safety.
4. **Validation & testing** – unit tests, regression tests, physics benchmarks.
5. **Performance** – computational complexity, memory, parallelism, GPU acceleration.
6. **Integration** – with existing QuantumVerse subsystems (MetricTensor, DiscoveryEngine, UI4D, etc.).
7. **Extensibility** – can other theories/plugins be added later?
8. **Known pitfalls** – what could break silently? What is the failure mode?

For each node, the skill asks a specific question, provides a recommended answer (grounded in code if possible), waits for user’s response, then proceeds to the next branch or sub‑branch.

### Step 2: Code exploration for factual questions

If a question can be answered by reading the existing codebase (e.g., “Does MetricTensor already support complex metrics?”), the skill will **not** ask the user. Instead it will:

1. Use `grep`, `find`, or a language‑aware tool (if available) to search the code.
2. Report the finding.
3. Optionally ask the user whether that changes their plan.

Example: *“I checked `src/spacetime/MetricTensor.h` and found `virtual Complex4D metricComponent(...)` – complex metrics are already supported. Do you still need to add a new class?”*

### Step 3: Physics‑specific probes (for relevant designs)

When the plan involves general relativity, quantum gravity, or any physics beyond Newtonian, the skill injects additional questions:

- **Coordinate invariance**: “Does this quantity transform as a tensor? How will you verify that?”
- **Causal structure**: “Will this feature preserve causality (no closed timelike curves)? What if it doesn’t?”
- **Conservation laws**: “Does your algorithm conserve energy‑momentum? What is the expected drift?”
- **Singularity handling**: “What happens when geodesic integration approaches a curvature singularity?”
- **Dimensional analysis**: “Are the units consistent? Check each term in the equation.”
- **Benchmark**: “What existing test (Mercury precession, light deflection, frame‑dragging) will you use to validate this?”

### Step 4: Risk and mitigation

After each major decision point, the skill asks:

*“What is the single biggest risk with this decision? How can we mitigate it?”*

If the user doesn’t have an answer, the skill provides typical risks based on similar features in QuantumVerse.

### Step 5: Summary and follow‑up

At the end of the session, the skill produces a **structured summary** of:

- Decisions made (with agreed answers)
- Open questions or unresolved branches
- Actionable recommendations
- Recommended next steps (e.g., “implement a prototype”, “write a failing test first”, “update documentation”)

---

## Example Session (Abridged)

**User:** “Grill me on my plan to add a new `WormholeMetric` class that supports traversable Lorentzian wormholes.”

**Assistant (grill-me):**  
I’ll grill you systematically. First question: **What metric ansatz do you plan to use? (e.g., Morris‑Thorne, Teo, or a numerical solution?)**

*My recommendation:* Start with the Morris‑Thorne metric because it’s well‑studied and has analytic expressions for embedding diagrams and geodesics. But note its throat violates the null energy condition – you’ll need exotic matter.  

**User:** Morris‑Thorne.

**Assistant:** Good. Next: **What coordinate system will you use for numerical integration? (isotropic, Schwarzschild‑like, proper distance?)**

*Recommendation:* Use `(t, r, θ, φ)` with `r` as the radial coordinate in the Morris‑Thorne line element `ds² = -e^{2Φ(r)} dt² + dr²/(1 - b(r)/r) + r² dΩ²`. This is standard and matches existing `MetricTensor` interfaces.  

**User:** Agreed.

**Assistant:** *(code exploration)* I’ve checked `src/spacetime/MetricTensor.h` – it already supports diagonal metrics via `getMetricComponent()`. You can inherit from `DiagonalMetric`. Good.

Now: **How will you handle the throat where `1 - b(r)/r` becomes zero?** That’s a coordinate singularity similar to a horizon.

*Recommendation:* In the geodesic integrator, you must stop at `r = b(r)` and apply a continuation through the throat (e.g., switch to a different radial coordinate like `l` proper distance). Alternatively, use a horizon‑penetrating coordinate system (e.g., Eddington‑Finkelstein‑like for wormholes). Which do you prefer?

**User:** Proper distance coordinate.

**Assistant:** That will require transforming the metric. Have you implemented a coordinate transformation class? If not, consider adding `CoordinateTransform` in `src/spacetime/`.  

Let’s continue… *(the session proceeds through numerical integration, validation against GR tests, performance, UI requirements, etc.)*

At the end, the assistant produces a summary:

```
SUMMARY:
- Metric ansatz: Morris‑Thorne (user choice)
- Coordinates: (t, r, θ, φ) with proper‑distance transformation near throat
- Exotic matter: will be handled via stress‑energy tensor plugin (pending implementation)
- Validation: test null geodesic through throat; compare deflection angle with known wormhole paper (reference)
- Risk: Null energy condition violation may cause numerical instability at throat → mitigation: add softening and fallback to analytical continuation
- Next step: implement CoordinateTransform and a simple test geodesic crossing the throat.
```

---

## Customization

The skill can be invoked with additional flags to focus the interrogation:

| Flag | Focus |
|------|-------|
| `--physics` | Emphasize physical assumptions, units, conservation laws, validation tests |
| `--performance` | Grill on O() complexity, memory, parallelization, GPU offloading |
| `--security` | (If multi‑user or blockchain features) – authentication, data integrity |
| `--api` | Focus on interface design, backward compatibility, deprecation strategy |
| `--no-code` | Skip automatic code exploration (e.g., when codebase not available) |

Example: `grill-me --physics --performance "my new geodesic integrator"`

---

## Return Codes (for automation)

| Code | Meaning |
|------|---------|
| 0 | Shared understanding reached; decisions clear. |
| 1 | Session incomplete (user stopped). |
| 2 | Contradiction found (user’s answers conflict with codebase). |
| 3 | Critical risk unmitigated (recommend further analysis). |

---

## Integration with Other Skills

- **validate-physics** – can be invoked to check quantitative claims (e.g., “the new metric reproduces GR at large r” – run `validate-physics --metric wormhole --large-r-limit`).
- **physics-module** – after design approval, the assistant can scaffold the new class.
- **deploy-release** – if the plan includes a release, the grill may include deployment safety checks.

---

**Upgrade Summary**:
- Systematic decision‑tree approach (domain‑specific).
- Code exploration to answer factual questions automatically.
- Physics‑specific probes (dimensional analysis, conservation laws, validation).
- Risk and mitigation required per decision.
- Structured summary and actionable next steps.
- Optional focus flags.
- Integration with existing QuantumVerse validation and module creation skills.

This turns a simple “ask questions” script into a **rigorous design review assistant** suitable for complex scientific software projects.