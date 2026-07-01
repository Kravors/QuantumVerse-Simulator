# TDD Workflow for QuantumVerse

When implementing a new physics component:

1. **Write the test first** in `tests/unit/test_new_component.cpp`
2. **Run the test** - it should fail
3. **Implement minimal code** to pass the test
4. **Refactor** while keeping tests green
5. **Add integration tests** for interactions
6. **Run validation tests** against analytical solutions
7. **Benchmark** performance
8. **Commit** with clear message referencing the test