// tests/test_tour_manager.cpp
// TourManager smoke test: scans data/tours/ and loads black_hole_basics.json.
#include "education/TourManager.h"

#include <cstdio>
#include <string>

using namespace quantumverse;

namespace {

int g_failures = 0;

#define CHECK(cond) do {                                                     \
    if (!(cond)) {                                                           \
        ++g_failures;                                                        \
        std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    }                                                                        \
} while (0)

} // namespace

int main() {
    TourManager& m = TourManager::instance();
    CHECK(m.initialize("data/tours/"));

    const auto names = m.listTours();
    std::printf("TourManager: found %zu tour(s)\n", names.size());
    for (const auto& n : names) std::printf("  - %s\n", n.c_str());

    CHECK(m.hasTour("black_hole_basics"));
    CHECK(m.count() >= 1);

    auto tour = m.getTour("black_hole_basics");
    CHECK(tour != nullptr);
    if (tour) {
        std::printf("Loaded tour: %s (%s), %zu steps\n",
                    tour->id.c_str(), tour->title.c_str(), tour->steps.size());
        CHECK(tour->validate());
        CHECK(!tour->steps.empty());
    }

    std::printf("TourManager tests: %d failures\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}