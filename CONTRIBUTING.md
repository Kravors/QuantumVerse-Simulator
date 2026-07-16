# Contributing to QuantumVerse

## Coding Style
- C++17, MSVC /W4 /WX, no warnings allowed
- Unused parameters: `(void)param;`
- Indentation: 4 spaces, no tabs

## Testing
- Every new feature must include a unit test
- Run `ctest -C Release` before submitting a PR
- Keep performance benchmarks within 1.3× baseline

## Pull Requests
- Link to an issue
- Describe what changed and why
- Verify CI passes (GitHub Actions)
