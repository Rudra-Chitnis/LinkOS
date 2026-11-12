# Development

## Requirements

- CMake 3.25 or newer
- A C17 compiler (MSVC 2022 on Windows; GCC or Clang on Linux)
- GNU Make on Linux, or Visual Studio 2022 on Windows
- Python 3 (only for the CLI integration test when tests are enabled)

## Configure, build, and test

Windows, from a Visual Studio 2022 developer shell:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
```

Linux (Ubuntu 24.04 reference):

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release
```

Build output is kept under `build/`. The CI workflow pins Windows to the
`windows-2022` image for the Visual Studio 2022 generator presets and uses
Ubuntu 24.04 for Linux. It runs configure, build, and test in Debug and Release
for pushes and pull requests; hosted results have not been observed yet.
The loopback tests bind to IPv4 loopback on port zero and do not use public network
access or machine-specific addresses.

## Current scope

The CLI supports `help`, `--version`, `doctor`, and a foreground `start` lifecycle.
The runtime handles Ctrl+C (and SIGTERM on Linux), but does not yet open a network
listener. Discovery, protocol handling, configuration persistence, and file sharing
are not implemented. The internal socket API is single-thread confined; use its
bounded poll operation for multiple live sockets, and close all handles before
network cleanup.
