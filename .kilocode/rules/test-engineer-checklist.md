# Test Engineering Checklist

**Version**: 0.1.1  
**Last Updated**: 2026-06-27  
**Project**: QuantumVerse Simulator

Before marking a test task complete:

- [ ] Unit test covers all public methods
- [ ] Edge cases tested (NaN, Inf, zero, large values)
- [ ] Mocked external dependencies
- [ ] Integration test with real dependencies
- [ ] Validation against analytical solution (if applicable)
- [ ] Performance benchmark written
- [ ] Test passes in CI (Linux/Windows/macOS)
- [ ] Memory leak check passed (Valgrind/ASan)
- [ ] Coverage meets threshold for module (≥80% for core modules)
- [ ] Test is deterministic (no flakiness)
- [ ] Clear assertion messages
- [ ] Documentation updated (if API changed)

## Test Categories

| Category | Test File Pattern | Description |
|----------|-------------------|-------------|
| Unit Tests | `tests/test_*.cpp` | Individual component tests |
| Integration Tests | `tests/test_integration*.cpp` | Cross-module tests |
| Validation Tests | `tests/test_*validation*.cpp` | GR validation tests |
| Performance Tests | `tests/test_*benchmark*.cpp` | Performance benchmarks |
| Physics Tests | `tests/test_*gravity*.cpp` | Physics accuracy tests |

## Running Tests

```bash
# Build all tests
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build build --parallel

# Run all tests
cd build && ctest --output-on-failure

# Run specific test category
ctest -R "Validation" --output-on-failure
ctest -R "Performance" --output-on-failure
```