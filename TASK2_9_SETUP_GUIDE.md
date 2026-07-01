# Task 2.9: LaTeX Paper Generator — Setup Guide

## Overview

The `PaperGenerator` module auto-generates publication-ready LaTeX documents from QuantumVerse discovery results. It supports APS and Nature templates, optional Jinja2-based rendering, and can invoke `pdflatex` to produce PDFs.

## Dependencies

### Required
- **C++17** compiler (MSVC 2019+, GCC 7+, Clang 6+)
- **CMake** 3.16+
- **LaTeX distribution** (TeX Live, MiKTeX, or MacTeX) for PDF compilation

### Optional
- **Python 3.8+** with Jinja2 for full template rendering
  ```bash
  pip install -r python/requirements_task2_9.txt
  ```

## Building

The `PaperGenerator` is built as part of the main `quantumverse` static library.

```bash
cd f:/syyyy
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --target test_paper_generator
```

## Running Tests

```bash
cd build
ctest -R PaperGeneratorTest -C Release
```

Expected output: `Passed`

## Usage Example

### Minimal C++ Example

```cpp
#include "paper/PaperGenerator.h"
#include "discovery/DiscoveryEngine.h"

int main() {
    quantumverse::PaperGenerator generator;
    quantumverse::PaperOptions opts;
    opts.output_dir = "./output";
    opts.template_style = "aps";
    opts.compile_pdf = false;  // Set true if pdflatex available
    generator.setOptions(opts);

    // Create a dummy discovery result
    quantumverse::DiscoveryResult discovery;
    discovery.type = "Spacetime Anomaly";
    discovery.confidence = 0.96;
    discovery.description = "Unexpected curvature near rotating black hole";
    discovery.fieldEquation = "R_{μν} - ½ R g_{μν} + Λ g_{μν} = 8π T_{μν}";
    discovery.location = {0.0, 1.5, 0.0, 0.0};  // t, x, y, z

    quantumverse::Hypothesis hypothesis;
    hypothesis.parameters = {{"M", 1.4}, {"a", 0.3}};

    auto result = generator.generateFromDiscovery(discovery, hypothesis);
    if (!result.error_message.empty()) {
        std::cerr << "Error: " << result.error_message << std::endl;
    } else {
        std::cout << "TeX generated: " << result.tex_path << std::endl;
        if (result.compilation_success) {
            std::cout << "PDF generated: " << result.pdf_path << std::endl;
        }
    }
    return 0;
}
```

### Python Jinja2 Backend

If Python and Jinja2 are available, the generator uses them for full templating. Otherwise, it falls back to stub string substitution (minimal LaTeX).

Check backend status:

```cpp
bool available = PaperGenerator::isLaTeXAvailable();
std::cout << "LaTeX available: " << available << std::endl;
```

## Output

- `discovery.tex` — main LaTeX file
- `references.bib` — bibliography (hardcoded Einstein reference)
- `discovery.pdf` — compiled PDF (if `compile_pdf=true` and `pdflatex` found)

## Custom Templates

```cpp
std::string my_template = R"(
\documentclass{article}
\begin{document}
\title{%%TITLE%%}
\author{%%AUTHOR%%}
\maketitle
%%CONTENT%%
\end{document}
)";
generator.createCustomTemplate("mytemplate", my_template);
generator.getOptions().template_style = "mytemplate";
```

## Troubleshooting

### `pdflatex` not found
Ensure LaTeX is installed and in PATH:
```bash
pdflatex --version
```
On Windows, add MiKTeX or TeX Live bin directory to PATH.

### Python/Jinja2 not detected
Install Jinja2:
```bash
pip install jinja2
```
Rebuild (the `HAVE_PYTHON` flag is auto-detected at configure time if pybind11 is found).

### `std::filesystem` errors
Requires C++17. Ensure CMake sets `CMAKE_CXX_STANDARD=17`. On older compilers, use `-lstdc++fs` (GCC) or update MSVC.

## Files

- `src/paper/PaperGenerator.h` — Public API
- `src/paper/PaperGenerator.cpp` — Implementation (PIMPL, stub/Jinja2 backends)
- `tests/test_paper_generator.cpp` — Unit tests
- `python/requirements_task2_9.txt` — Python dependencies

## Status

✅ **Implementation complete** — Core functionality working, tests passing.
🚧 **Future work**: Figure generation (matplotlib integration), citation manager (BibTeX query), DiscoveryEngine integration.
