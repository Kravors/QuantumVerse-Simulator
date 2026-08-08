# generate-report.ps1
# Categorizes errors from extracted-errors.txt and produces a Markdown report.

$ErrorFile = "F:\syyyy\failure-analysis\extracted-errors.txt"
$OutputReport = "F:\syyyy\failure-analysis\report.md"

# Read all error lines
$lines = Get-Content $ErrorFile -Encoding UTF8

# Define category patterns
$categories = @{
    "Compiler warnings as errors (Werror)" = @('\[-Werror=', 'warning: .* treated as error', 'error: .* -Werror')
    "CMake configuration errors" = @('CMake Error', 'CMakeLists.txt:', 'Could NOT find', 'Generator', 'Visual Studio', 'CMAKE_PREFIX_PATH')
    "MSBuild / MSVC compilation errors" = @('MSBUILD : error', 'error C', 'fatal error C', 'LNK')
    "Missing includes / headers" = @('error: .* does not name a type', 'unknown type name', 'no member named', 'did you mean')
    "Linking errors" = @('undefined reference', 'unresolved external symbol', 'LNK2019', 'LNK1120')
    "Runtime / test failures" = @('FAIL: ', 'Assertion', 'Segmentation fault', 'exit code', 'Headless run failed', 'performance gate check')
    "Missing dependencies" = @('librdkafka not found', 'CURL not found', 'Could NOT find WrapVulkanHeaders', 'glslangValidator not found')
    "CI / environment issues" = @('Node.js 20 is deprecated', 'runner image', 'timeout', 'disk space')
    "Miscellaneous" = @()  # fallback
}

# Initialize counters and samples
$counts = @{}
$samples = @{}
foreach ($cat in $categories.Keys) { $counts[$cat] = 0; $samples[$cat] = @() }

# Scan each line
foreach ($line in $lines) {
    $matched = $false
    foreach ($cat in $categories.Keys) {
        if ($cat -eq "Miscellaneous") { continue }
        $patterns = $categories[$cat]
        foreach ($pattern in $patterns) {
            if ($line -match $pattern) {
                $counts[$cat]++
                if ($samples[$cat].Count -lt 5) { $samples[$cat] += $line.Trim() }
                $matched = $true
                break
            }
        }
        if ($matched) { break }
    }
    if (-not $matched) {
        $counts["Miscellaneous"]++
        if ($samples["Miscellaneous"].Count -lt 5) { $samples["Miscellaneous"] += $line.Trim() }
    }
}

# Sort by count descending
$sorted = $counts.GetEnumerator() | Sort-Object Value -Descending

$tableLines = @()
$total = ($counts.Values | Measure-Object -Sum).Sum
foreach ($item in ($sorted | ForEach-Object { $_ })) {
    $pct = if ($total -gt 0) { [math]::Round(($item.Value / $total) * 100, 1) } else { 0 }
    $tableLines += "| $($item.Key) | $($item.Value) | $($pct)% |"
}
$tableBlock = $tableLines -join "`n"

$report = @"
# Root‑Cause Report for QuantumVerse-Simulator CI Failures
Generated from $(($lines | Measure-Object -Line).Lines) extracted error lines across 479 failed jobs (from 215 workflow runs).

---

## 📊 Overall Failure Distribution

| Category | Count | Percentage |
|----------|-------|------------|
$($tableBlock)

---

## 🔍 Representative Log Snippets

$(
    $sorted | ForEach-Object {
        $cat = $_.Key
        $samp = $samples[$cat]
        $snippet = if ($samp) { $samp -join "`n`n" } else { "*No samples*" }
        @"

### $cat

$($snippet)
"@
    }
)

---

## 🛠️ Recommended Fixes

Based on the most frequent categories, here are actionable remediation steps:

### 1. Compiler warnings as errors (`-Werror`)
Many test files (e.g., `test_singularity_handler.cpp`, `test_exact_solutions.cpp`) have unused variables or parentheses warnings that break the build because `-Werror` is enabled.
**Fix**: Either remove `-Werror` from the CMake flags (temporarily) or fix the warnings. For unused variables, add `(void)var;` or use `[[maybe_unused]]`. For parentheses, add explicit braces.

### 2. CMake configuration errors (Windows)
The Windows build fails due to generator mismatches or missing dependencies. Even though you removed `-G "Visual Studio 17 2022"`, check that all workflow files no longer contain it. Also verify that `CMAKE_PREFIX_PATH` is correctly set to the Qt `lib/cmake` folder. Example:
```yaml
-DCMAKE_PREFIX_PATH="$env:QT_ROOT_DIR\lib\cmake"
```

### 3. MSBuild / MSVC errors
These are usually syntax or missing include errors. Look for `error C` lines in the raw logs to pinpoint the file and line.

### 4. Missing includes / headers
The error `error: 'unique_ptr' in namespace 'std' does not name a template type` (already fixed) is a classic example. Ensure all headers that use smart pointers include `<memory>`.

### 5. Linking errors
`undefined reference` and `unresolved external symbol` indicate missing libraries or wrong link order. Check that `target_link_libraries` orders dependencies correctly (e.g., glad before GLFW).

### 6. Runtime / test failures
If tests fail with exit codes, run them locally with verbose output. For performance gates, adjust thresholds or fix performance regressions.

### 7. Missing dependencies (non‑fatal)
These are mostly informational (e.g., librdkafka, CURL, Vulkan). They don't cause build failures but may limit functionality. You can install them if needed, or ignore.

### 8. CI / environment issues
The Node.js deprecation warning is harmless. Ignore it for now.

---

## 📌 Next Steps

1. **Check Windows generator removal** – Search your workflows for `-G "Visual Studio` and ensure none remain.
2. **Fix `-Werror` warnings** – Either disable `-Werror` globally or patch the warnings in the test files.
3. **Review raw logs** for specific `error C` or `undefined reference` lines that are not covered by the categories above.
4. **Rerun CI** after each fix to validate.

---

## 📎 Report Metadata

- Total workflow runs analyzed: 300
- Failed runs: 215
- Failed jobs: 479
- Error lines extracted: $(($lines | Measure-Object -Line).Lines)

"@

# Write report
$report | Out-File -FilePath $OutputReport -Encoding UTF8
Write-Host "Report written to $OutputReport"
