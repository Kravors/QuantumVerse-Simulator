# Test Engineering Checklist

Before marking a test task complete:

- [ ] Unit test covers all public methods
- [ ] Edge cases tested (NaN, Inf, zero, large values)
- [ ] Mocked external dependencies
- [ ] Integration test with real dependencies
- [ ] Validation against analytical solution (if applicable)
- [ ] Performance benchmark written
- [ ] Test passes in CI (Linux/Windows/macOS)
- [ ] Memory leak check passed (Valgrind/ASan)
- [ ] Coverage meets threshold for module
- [ ] Test is deterministic (no flakiness)
- [ ] Clear assertion messages
- [ ] Documentation updated (if API changed)