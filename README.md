# ChessComputer

Eine in C++20 geschriebene Schach-Engine mit Zuggenerierung, Evaluation und Suche.

## Build

**Voraussetzungen:** CMake ≥ 3.20, C++20-kompatibler Compiler (MSVC oder Clang)

```bash
# Release-Build (MSVC)
cmake --preset msvc-release
cmake --build out/build/msvc-release

# Debug-Build mit Sanitizern (Clang)
cmake --preset clang-debug
cmake --build out/build/clang-debug
```

# Schach spielen (CLI)
./out/build/msvc-release/ChessPlay

# Tests ausführen
./out/build/msvc-release/ChessTests
