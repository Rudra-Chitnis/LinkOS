# Roadmap

## M0 — Foundation
- [x] Portable C17 + CMake
- [x] CLI dispatch and help/version/doctor
- [x] Platform config path adapter
- [x] Smoke tests

## M1 — Lifecycle and configuration
- [ ] Structured config file with safe defaults
- [ ] Log levels and optional log file
- [x] Foreground runtime lifecycle and graceful shutdown
- [x] Linux Debug and Release builds and CLI/SIGINT/SIGTERM checks (Ubuntu 24.04 WSL)
- [x] Windows Debug and Release builds and CLI console-control shutdown checks (VS 2026 local; harness uses CTRL_BREAK)
- [ ] Hosted Windows CI results (workflow configured; results pending)

## M2 — Network primitives
- [x] Windows Winsock2 TCP implementation (native VS 2026 build/tests passed)
- [x] Linux POSIX TCP implementation (Debug/Release build and tests passed under Ubuntu 24.04 WSL)
- [x] Opaque handles, portable errors, total send deadlines, and cleanup/resource limits
- [x] Multi-socket event-loop poll and partial-send API (Linux tests passed)
- [x] Loopback tests for simultaneous peers, bidirectional data, partial send, close, timeout, cleanup, and capacity recovery
- [x] Concurrency architecture decision documented in ADR 0002
- [x] Linux and Windows local Debug/Release build and test verification
- [ ] Hosted Windows/Linux CI results for this working tree

## M2.5 — Concurrency and lifecycle hardening
- [x] Select a single-thread event-loop model and document thread confinement
- [x] Bound socket count, report partial-send progress, and poll multiple handles
- [x] Exercise concurrent peer readiness, resource-cap recovery, and shutdown refusal
- [x] Native Windows Debug/Release builds and concurrency tests (VS 2026 local)
- [ ] Hosted Windows CI results (workflow configured; results pending)

## M3 — Discovery and protocol
- [ ] Review proposed UDP multicast discovery design in ADR 0003
- [ ] Implement bounded versioned discovery datagrams and IPv4 per-interface membership
- [ ] Add interface-change reconciliation, leases, duplicate handling, and discovery configuration
- [ ] Add deterministic parser/lifecycle tests and local-only platform integration tests
- [ ] Keep discovered peers explicitly unverified; identity and pairing belong to M4

## M4 — Protocol and secure sessions
- [ ] Versioned application framing and bounded decode
- [ ] Select and integrate maintained secure transport
- [ ] Pairing, authentication, trust storage, and revocation

## M5 — Shared directories and browsing
- [ ] Explicit shared roots and permissions
- [ ] Safe path resolution and metadata listing

## M6 — File transfer
- [ ] Chunked download and explicitly authorized upload
- [ ] Progress, cancellation, collision behavior, integrity verification

## M7 — Release readiness
- [ ] Windows/Linux packaging and clean-machine install tests
- [ ] Release workflow and end-to-end cross-platform LAN verification
