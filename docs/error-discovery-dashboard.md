# QuantumVerse Error Discovery Dashboard

## Overview

This dashboard tracks the status of all error discovery and prevention components.

## Tool Status

| Tool | Status | Last Run | Next Run |
|------|--------|----------|----------|
| CBMC | Active (PR + Nightly) | - | Continuous |
| Clang-Tidy | Active (PR) | - | Every Push |
| Differential Testing | Active (PR) | - | Every Push |
| Semgrep SAST | Configured | - | PR + Nightly |
| Valgrind Memcheck | Configured | - | Nightly |
| libFuzzer | Configured | - | PR + Continuous |
| Headless UI Mode | Active | - | Every Push |
| Root-Cause Analysis | Ready | Failure-triggered | On-demand |

## CI/CD Integration

### GitHub Actions Workflows

1. **ci.yml** - Full diagnostic suite on every push
2. **formal-verification.yml** - CBMC and differential testing on PRs
3. **security-scan.yml** - Semgrep SAST on PRs
4. **coverage.yml** - Coverage enforcement (95% line / 90% branch)

### Build Options

```bash
# Build with all verification tools
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_BUILD_TESTS=ON
cmake --build build --parallel

# Run all tests
cd build && ctest --output-on-failure

# Build with sanitizers
cmake -B build -DQUANTUMVERSE_USE_ASAN=ON -DQUANTUMVERSE_USE_UBSAN=ON
cmake --build build --parallel
```

## Running Verification

### CBMC Verification

```bash
# Build verification harnesses
cmake -B build -DQUANTUMVERSE_ENABLE_CBMC=ON
cmake --build build

# Run CBMC on specific harness
cbmc tests/verification/harness_metric.cpp \
  --bounds-check --pointer-check --unwind 10
```

### Differential Testing

```bash
# Build both variants
cmake -B build-double -DCMAKE_BUILD_TYPE=Release
cmake -B build-long-double -DCMAKE_BUILD_TYPE=Release -DQUANTUMVERSE_USE_LONG_DOUBLE=ON

cmake --build build-double
cmake --build build-long-double

# Compare outputs
./build-double/test_differential
./build-long-double/test_differential
```

### Headless UI Testing

```bash
# Run in headless mode for N frames
./build/Release/quantumverse_imgui --headless --frames 100

# Run in headless mode with default 100 frames
./build/Release/quantumverse_imgui --headless
```

### Golden Master Tests

```bash
# Generate golden master
python3 scripts/golden_master_test.py EarthRenderer --update-golden

# Run comparison
python3 scripts/golden_master_test.py EarthRenderer
```

### Chaos Engineering

```bash
# Install chaos toolkit
pip install chaostoolkit

# Run experiment
chaos run chaos/experiment.json

# Run with journal output
chaos run chaos/experiment.json --journal-output journal.json
```

### eBPF Monitoring

```bash
# Monitor a running process
python3 scripts/monitor_ebpf.py <pid> 60

# On Linux with bpftrace installed
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_mmap { @ = count(); }'
```

### Root-Cause Analysis

```bash
# Analyze test failure
python3 scripts/analyze_root_cause.py build/TestResults.log TestName

# Run with git bisect
python3 scripts/analyze_root_cause.py build/TestResults.log TestName --bisect
```

## Success Metrics

| Metric | Target | Current |
|--------|--------|---------|
| Critical bugs found in CI | 0 | - |
| False positive rate | < 5% | - |
| Mean time to detect regression | < 2 hours | - |
| Mean time to root cause | < 1 hour | - |
| Line coverage | ≥ 95% | - |
| Branch coverage | ≥ 90% | - |

## Configuration

### Environment Variables

| Variable | Description | Required |
|----------|-------------|----------|
| `OPENAI_API_KEY` | API key for AI review | No |
| `CBMC_PATH` | Path to CBMC binary | No |
| `KLEE_PATH` | Path to KLEE binary | No |

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `QUANTUMVERSE_USE_ASAN` | OFF | AddressSanitizer |
| `QUANTUMVERSE_USE_TSAN` | OFF | ThreadSanitizer |
| `QUANTUMVERSE_USE_UBSAN` | OFF | UndefinedBehaviorSanitizer |
| `QUANTUMVERSE_USE_FUZZER` | OFF | libFuzzer targets |
| `QUANTUMVERSE_USE_BOOST_MULTIPRECISION` | OFF | Formal verification tests |
| `QUANTUMVERSE_ENFORCE_TIDY_ERRORS` | ON | Treat clang-tidy warnings as errors |
| `QUANTUMVERSE_USE_COVERAGE` | OFF | Enable code coverage instrumentation |
| `QUANTUMVERSE_ENABLE_CBMC` | OFF | Enable CBMC formal verification |
| `QUANTUMVERSE_ENABLE_KLEE` | OFF | Enable KLEE symbolic execution |
| `QUANTUMVERSE_USE_GOLDEN_MASTER` | OFF | Enable golden master tests |
| `QUANTUMVERSE_USE_LONG_DOUBLE` | OFF | Build with long double precision |
| `QUANTUMVERSE_USE_RAPIDCHECK` | ON | Enable property-based tests |

## Alert Thresholds

| Check | Threshold | Alert |
|-------|-----------|-------|
| Test failure | Any | GitHub Actions failed |
| CBMC violation | Any | Verification failed |
| Memory leak (eBPF) | > 1 KB/hour | Performance issue |
| Coverage drop | > 5% | Coverage regression |
| Chaos experiment | Any failure | Resilience issue |

## Resources

- [CBMC Documentation](https://www.cprover.org/cbmc/)
- [KLEE Symbolic Execution](https://klee.se/docs/)
- [Chaos Toolkit Documentation](https://chaostoolkit.org/)
- [ImageHash for Golden Master](https://github.com/JohannesBuchner/imagehash)