/**
 * @file WasmBridge.h
 * @brief C-compatible API layer exposing QuantumVerse core to JavaScript.
 *
 * Uses extern "C" functions callable via emscripten's cwrap/ccall from JS.
 * Avoids EMSCRIPTEN_BINDINGS/embind dependency for simpler linking.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "../spacetime/KerrMetric.h"
#include "../physics/PhysicsValidator.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Opaque handles ---------------------------------------------------------

typedef struct WasmBridge WasmBridge;

/// Create a new bridge instance.
WasmBridge* qv_bridge_create(void);

/// Destroy a bridge instance.
void qv_bridge_destroy(WasmBridge* bridge);

// --- Metric / Validation ----------------------------------------------------

/// Create a Kerr metric. Returns a JSON string: {"mass":..., "spin":...}
const char* qv_bridge_create_metric(WasmBridge* bridge, double mass, double spin);

/// Validate a proposed black hole state. Returns JSON string.
const char* qv_bridge_validate_state(WasmBridge* bridge, double mass, double spin, double charge);

/// Get the current physics warning flag (1 = invalid spacetime).
int qv_bridge_physics_warning(WasmBridge* bridge);

/// Get the current mass (for reading remote-synced state).
double qv_bridge_get_mass(WasmBridge* bridge);

/// Get the current spin (for reading remote-synced state).
double qv_bridge_get_spin(WasmBridge* bridge);

/// Get the current charge (for reading remote-synced state).
double qv_bridge_get_charge(WasmBridge* bridge);

/// Set the black hole mass (for remote sync from other clients).
void qv_bridge_set_mass(WasmBridge* bridge, double mass);

/// Set the black hole spin (for remote sync from other clients).
void qv_bridge_set_spin(WasmBridge* bridge, double spin);

/// Set the black hole charge (for remote sync from other clients).
void qv_bridge_set_charge(WasmBridge* bridge, double charge);

// --- RL Discovery -----------------------------------------------------------

/// Run one RL discovery step. Returns JSON string with deltas and veto flag.
const char* qv_bridge_run_discovery_step(WasmBridge* bridge, double mass, double spin, double charge);

// --- LLM Co-Pilot -----------------------------------------------------------

/// Send anomaly data to the LLM. Result delivered via callback registered
/// with qv_bridge_set_llm_callback().
void qv_bridge_analyze_anomaly(WasmBridge* bridge, const char* anomalyData);

/// Register a JS callback (function pointer) for LLM explanations.
typedef void (*LLMCallback)(const char* explanation);
void qv_bridge_set_llm_callback(WasmBridge* bridge, LLMCallback callback);

#ifdef __cplusplus
} // extern "C"
#endif
