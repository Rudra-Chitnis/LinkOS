# LinkOS

**A cross-platform, CLI-first peer-to-peer file sharing tool.**

LinkOS is being developed in portable C. The initial release targets Windows and Linux, with a shared application core and isolated platform-specific implementations.

## Current status

**Milestone 2 — Networking foundation in progress**

- [x] C17 project structure and CMake build
- [x] CLI command dispatch (`help`, `--version`, `doctor`)
- [x] Platform identification and per-user config path discovery
- [x] Minimal logging interface
- [x] CTest smoke tests
- [x] Foreground lifecycle with Ctrl+C shutdown handling
- [x] Windows/Linux CI workflow and Debug/Release presets (hosted results pending)
- [x] Portable TCP API and multi-socket loopback tests (Linux Debug/Release passed)
- [x] Single-threaded bounded event-loop readiness with partial-send reporting
- [x] M1 Linux Debug/Release commands and signal shutdown checks passed
- [x] Native Windows Debug/Release builds and console control-event checks (VS 2026 local)
- [ ] Hosted Windows CI results (workflow configured; pending)
- [ ] Device discovery and application protocol
- [ ] Pairing, authenticated sessions, and explicit shared folders
- [ ] File browsing and transfers

TCP socket primitives and a bounded single-thread readiness loop are implemented,
but discovery, application protocol, secure peer sessions, and file transfer are
not implemented.

The canonical product charter is [docs/PROJECT_VISION_AND_SPEC.md](docs/PROJECT_VISION_AND_SPEC.md).

## Build on Windows

Requirements:
- CMake 3.25+
- Visual Studio 2022 with **Desktop development with C++** workload (includes MSVC C compiler), or another C17 compiler
- Git (recommended)

From a Developer PowerShell for VS 2022:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug
.\build\windows-debug\Debug\linkos.exe --version
.\build\windows-debug\Debug\linkos.exe doctor
```

If using a single-configuration generator such as Ninja, configure with a suitable C compiler and use the `windows-ninja` preset only when `cl.exe` is available on PATH.

## CLI

```text
linkos --version
linkos help
linkos doctor
linkos start
```

`start` runs in the foreground and shuts down on Ctrl+C (and SIGTERM on Linux).
It does not open a network listener; discovery and file sharing are not implemented.

See [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) for build and test instructions and
[docs/SECURITY.md](docs/SECURITY.md) for the current security status.

## Design rules

1. Keep the application core portable C17.
2. Put OS-specific APIs behind `src/platform/` interfaces.
3. Never expose unrestricted filesystem access.
4. Only explicitly approved shared directories may be browsed.
5. No remote command execution.
6. Treat all network input as untrusted; validate message lengths and paths.
7. Add tests with every meaningful feature.

## Planned layout

```text
include/linkos/       Public internal interfaces
src/cli/              CLI parsing and command dispatch
src/core/             Logging and shared application logic
src/platform/         OS-specific adapters
src/discovery/        LAN device discovery (future)
src/net/              TCP socket API and bounded readiness poller
src/protocol/         Framing and protocol messages (future)
src/fs/               Approved shared-root handling (future)
src/transfer/         Upload/download engine (future)
tests/                Automated tests
docs/                 Architecture and protocol documentation
packaging/            Installers and release packaging (future)
```

## Versioning

The project currently starts at `0.1.0`. Release artifacts will be published through GitHub Releases. Windows and Linux will have separate native builds from the same tagged source version.
