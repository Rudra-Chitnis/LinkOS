# ADR 0001: Portable TCP socket API

## Status

Accepted for the M2 TCP foundation. Concurrency choices are extended by ADR 0002.
Native Windows Debug/Release builds and tests passed locally with VS 2026; hosted
VS2022 CI results remain pending.

## Context

LinkOS needs TCP listener, connect, accept, send, receive, and close operations on
Windows and Linux. Native socket types and lifecycle APIs differ, and the shared
CLI/core must not depend on `SOCKET`, file descriptors, or OS error constants.

## Options considered

1. Expose native socket handles and let callers use platform code.
2. Use a cross-platform networking dependency.
3. Keep a small internal C adapter with opaque public handles and common errors.

## Decision

Use the internal C adapter in `src/net/socket.c`, exposed through `linkos/net.h`.
Keep platform handle types private, use Winsock2 on Windows and POSIX sockets on
Linux, perform readiness waits with `select`, and put a timeout on accept/connect/
send/receive waits. Binding accepts a host and port; port zero asks the OS for an
ephemeral port. Streams remain raw byte streams and do not define a protocol.

The API is currently single-threaded and process-global. Initialization is
idempotent; cleanup refuses to run while sockets remain open. Callers own each
opaque socket and close it before cleanup. Blocking DNS resolution remains outside
the connect timeout.

## Rationale and alternatives rejected

Native handles would spread OS-specific code into application modules. A third-party
dependency is unnecessary for the small primitive set and would add build and
distribution work. The adapter keeps the boundary explicit while preserving C17
and the repository's MIT distribution model.

`select` is available on both targets and is sufficient for the bounded socket set
described in ADR 0002. The process-global lifecycle is intentionally single-threaded
and must be revisited before multithreaded socket use.

## Portability, security, and licensing

Windows uses Winsock2 startup/cleanup, `SOCKET`, `ioctlsocket`, and `closesocket`;
Linux uses `int` descriptors, nonblocking `fcntl`, close-on-exec, `select`, and
`close`. No external dependency or license is introduced. All socket operations
are explicit and bounded by readiness timeouts; resolver calls can still block.
The layer provides no authentication, encryption, authorization, discovery, or
message framing. It must not be treated as a secure peer session.
