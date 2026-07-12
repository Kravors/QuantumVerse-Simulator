/**
 * @file LIGOAdapter.cpp
 * @brief Implementation of the LIGO gravitational-wave alert adapter
 *
 * The adapter is intentionally lightweight: alert delivery is driven by
 * simulateAlert() (tests, replay) or an external source calling it. A real
 * deployment would feed live GCN/Kafka events through the same callback.
 */

#include "LIGOAdapter.h"

namespace quantumverse {
// Header-only adapter; this translation unit exists so the class has a stable
// object file to link and to host future network-ingest logic.
} // namespace quantumverse
