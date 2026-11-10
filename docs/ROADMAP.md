# Roadmap

## M0 — Foundation
- [x] Portable C17 + CMake
- [x] CLI dispatch and help/version/doctor
- [x] Platform config path adapter
- [x] Smoke tests

## M1 — Lifecycle and configuration
- [ ] Structured config file with safe defaults
- [ ] Log levels and optional log file
- [ ] Foreground runtime lifecycle and graceful shutdown
- [ ] Windows/Linux CI builds

## M2 — Network primitives
- [ ] Winsock2 and POSIX socket adapters
- [ ] TCP loopback integration tests
- [ ] Timeouts and explicit error mapping

## M3 — Discovery and protocol
- [ ] UDP LAN discovery
- [ ] Versioned message framing
- [ ] Device identity and explicit pairing

## M4 — Shared folders and transfers
- [ ] Explicit shared roots
- [ ] Safe directory listing and metadata
- [ ] Download/upload with progress and cancellation
