# Build Guide

This project uses CMake and provides both direct commands and presets.
Run all commands from the repository root.

## Prerequisites

- CMake 3.21+
- A C++20-capable compiler (AppleClang/Clang)
- LLVM/Clang packages discoverable by CMake

## Build With Direct CMake Commands

Configure:

```bash
cmake -S . -B build
```

Build:

```bash
cmake --build build
```

Configure + build in one command:

```bash
cmake -S . -B build && cmake --build build
```

Convenience target:

```bash
make cinder
```

## Enable clang-tidy (Direct Command)

`clang-tidy` is optional and off by default.

```bash
cmake -S . -B build -DCINDER_ENABLE_CLANG_TIDY=ON
cmake --build build
```

If `clang-tidy` is not installed, CMake prints a warning.

## Build and Run Unit Tests (Direct Command)

Unit tests use GoogleTest and are enabled by default (`CINDER_BUILD_TESTS=ON`).

Configure + build with tests:

```bash
cmake -S . -B build -DCINDER_BUILD_TESTS=ON
cmake --build build
```

Run all tests:

```bash
ctest --test-dir build --output-on-failure
```

You can disable test targets when needed:

```bash
cmake -S . -B build -DCINDER_BUILD_TESTS=OFF
```

## Build With CMake Presets

Available configure presets:

- `debug`
- `release`
- `debug-clang-tidy`
- `debug-tests`
- `release-clang-tidy`

Available build presets:

- `build-debug`
- `build-release`
- `build-debug-clang-tidy`
- `build-debug-tests`
- `build-release-clang-tidy`

Available test presets:

- `test-debug`

Debug build:

```bash
cmake --preset debug
cmake --build --preset build-debug
```

Release build:

```bash
cmake --preset release
cmake --build --preset build-release
```

Debug with clang-tidy:

```bash
cmake --preset debug-clang-tidy
cmake --build --preset build-debug-clang-tidy
```

Release with clang-tidy:

```bash
cmake --preset release-clang-tidy
cmake --build --preset build-release-clang-tidy
```

Debug build with tests + run tests:

```bash
cmake --preset debug-tests
cmake --build --preset build-debug-tests
ctest --preset test-debug
```

## Output Locations

- Binary: `build/<preset>/bin/cinder` when using presets
- Binary: `build/bin/cinder` when using `-B build`
- Libraries: `build/<preset>/lib` or `build/lib`

## Quick Verification

After building:

```bash
./build/bin/cinder --compile -o /tmp/out ./tests/test.ci
```

If using presets, replace `build/bin/cinder` with the preset-specific path,
for example `build/debug/bin/cinder`.
