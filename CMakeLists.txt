cmake_minimum_required(VERSION 3.16)
project(QuantumVerse LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_library(dilaton INTERFACE)
target_include_directories(dilaton INTERFACE ${CMAKE_SOURCE_DIR}/src)

# Discovery Instrument Tests
add_executable(test_exoplanetary_hunter tests/discovery/test_exoplanetary_hunter.cpp)
target_link_libraries(test_exoplanetary_hunter PRIVATE dilaton)
add_test(NAME ExoplanetaryHunterTest COMMAND test_exoplanetary_hunter)

add_executable(test_galactic_rotation tests/discovery/test_galactic_rotation.cpp)
target_link_libraries(test_galactic_rotation PRIVATE dilaton)
add_test(NAME GalacticRotationTest COMMAND test_galactic_rotation)

add_executable(test_fine_structure tests/discovery/test_fine_structure_drift.cpp)
target_link_libraries(test_fine_structure PRIVATE dilaton)
add_test(NAME FineStructureTest COMMAND test_fine_structure)

add_executable(test_boson_star tests/discovery/test_boson_star.cpp)
target_link_libraries(test_boson_star PRIVATE dilaton)
add_test(NAME BosonStarTest COMMAND test_boson_star)

add_executable(test_neutron_star tests/discovery/test_neutron_star.cpp)
target_link_libraries(test_neutron_star PRIVATE dilaton)
add_test(NAME NeutronStarTest COMMAND test_neutron_star)

# Stress Test
add_executable(test_chaos tests/stress/test_chaos.cpp)
target_link_libraries(test_chaos PRIVATE dilaton)
add_test(NAME ChaosTest COMMAND test_chaos)