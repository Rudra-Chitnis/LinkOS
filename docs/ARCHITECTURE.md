# LinkOS architecture notes

## Current foundation

The executable is deliberately small. `src/main.c` delegates to the CLI command dispatcher. The command layer uses shared core services and a platform adapter. No socket API is exposed to the CLI.

The foreground lifecycle installs platform-specific shutdown handling in the
platform adapter: Windows console control events use an interlocked shutdown flag;
Linux handles SIGINT and SIGTERM with a `sig_atomic_t` flag. The shared command
loop polls that flag and owns startup/shutdown reporting. This currently provides
lifecycle behavior only; it does not start networking or background workers.

## TCP networking boundary

The M2 socket API uses opaque handles and keeps platform APIs private:

- Windows adapter: Winsock2 (`SOCKET`, `WSAStartup`, `closesocket`, `WSAGetLastError`)
- Linux adapter: POSIX sockets (`int`, `socket`, `close`, `errno`)
- Shared networking layer: listen/connect/send/receive abstractions and explicit error values

Do not typedef a Windows `SOCKET` as `int`. Keep startup/shutdown and error translation inside the platform adapter.

The M2 implementation is exposed through `include/linkos/net.h`. The public
interface uses opaque socket pointers, common errors, numeric ports, host strings,
bounded multi-socket readiness polling, and a nonblocking partial-send operation.
The implementation keeps `SOCKET`/`int`, Winsock/POSIX startup, nonblocking mode,
readiness waits, and close calls private. Streams are byte streams: send and receive
call boundaries are not message boundaries. The current model is a single-threaded
event loop with a 64-socket cap (listener included); all handles must stay on its
owning thread and be closed before cleanup. `send_all` has a total operation
timeout and reports success only after all bytes are sent. See ADRs 0001 and 0002.

There is no peer session registry yet. It will be added when protocol states give
it real lifecycle meaning. File I/O and hashing that could block the event loop may
later justify a bounded worker pool, with socket ownership remaining on the loop.

The final M2.5 audit found that the 64-handle cap was checked only when wrapping
new OS sockets. At capacity, listen/connect could create sockets and accept could
consume a queued connection before the wrapper rejected it. The API now checks
capacity before those operations; the wrapper retains its defensive check. A
regression test verifies queued connections remain available after an at-capacity
accept fails.

## Proposed M3 discovery boundary

M3 is proposed as a separate IPv4 UDP discovery service, using a narrow UDP
extension to the opaque socket API and the existing poll loop. It uses scoped
multicast joined and sent per eligible interface, with a small fixed-size
versioned wire message. The socket source address and advertised TCP port are endpoint hints;
they are never peer identity or proof of trust. The peer table is bounded and
entries expire on a short lease. Network-interface changes reconcile memberships
and remove endpoints tied to vanished interfaces. Discovery is configurable and
can be disabled. See proposed ADR 0003 for the format, lifecycle, limits, security
boundary, and implementation plan. This ADR is design-only; UDP discovery is not
implemented.

## Security constraints

- Pair devices explicitly before allowing file access.
- Share only user-selected directories.
- Canonicalize requested paths and prevent traversal outside approved roots.
- Do not follow symlinks outside a shared root.
- No remote shell or arbitrary command execution.
- Design protocol framing with bounded lengths, version negotiation, and request IDs.
