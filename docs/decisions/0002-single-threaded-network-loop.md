# ADR 0002: Single-threaded bounded network loop

## Status

Accepted for the current LAN transport foundation. Linux Debug/Release and sanitizer
verification passed. Native Windows VS 2026 Debug/Release and tests also passed;
hosted Windows CI remains pending.

## Context

The socket layer must support several simultaneous peer connections without making
socket ownership, shutdown, and cancellation difficult to reason about. The current
M2 implementation already uses nonblocking sockets and `select`, but had only
single-socket waits and process-global counters with an explicitly single-threaded
contract.

## Options considered

| Option | Benefits | Costs and risks |
|---|---|---|
| Single-threaded event loop | Portable readiness model; one owner for sockets and teardown; bounded resource use; simple deterministic tests | Handlers must stay short; one slow operation can delay all peers; `select` has a bounded descriptor set |
| Thread per connection | Straightforward blocking session code; peer failures are isolated | Unbounded threads and stacks; shared initialization/counts need synchronization; cross-platform thread and cancellation details |
| Bounded worker pool | Bounds thread count and supports blocking work | Requires task queues, backpressure, per-session synchronization, and shutdown coordination before there is work that benefits from it |
| Hybrid loop plus workers | Keeps network polling responsive while offloading slow work | Adds synchronization and ownership transfer; only justified when file I/O, hashing, or other expensive tasks arrive |

## Decision

Use a single-threaded event-loop contract for current transport code. Add
`linkos_net_poll` to wait for read/write readiness across up to 64 sockets, a
64-handle process resource cap, and `linkos_net_send_some` for one bounded,
nonblocking partial-send attempt. `send_all` loops over partial writes and returns
success only after the entire buffer is sent; it uses one total operation deadline.
Connect, accept, and receive readiness waits are bounded. Blocking DNS resolution
is outside the connect timeout.

All socket calls, handle ownership, initialization, and cleanup belong to one event
loop thread. A handle must not be operated on or closed concurrently. This design
supports up to 63 peer sockets plus a listener and keeps Winsock/POSIX types
private. It does not create threads or introduce a dependency.

Do not add a session registry yet. Until a protocol exists, a registry would only
duplicate the bounded socket list and invent lifecycle states without meaningful
peer semantics. Add explicit session states when protocol negotiation and peer
authorization define those transitions.

## Consequences and limits

The single-thread contract avoids races in process-global WSA startup and socket
counts. `select` and the public 64-socket limit fit the initial LAN scale and both
platforms. Linux also rejects descriptors at or above `FD_SETSIZE`. Event-loop
callbacks must not perform long filesystem work or long `send_all` calls; future
transfers may justify a bounded worker pool for file I/O while keeping socket
ownership on the loop thread.

Readiness is not a message boundary. Callers must handle partial receive results,
peer closure, timeouts, and `WOULD_BLOCK`. The transport has no security, protocol,
or session identity. Hosted Windows CI remains pending.

The final audit found that wrapper-count enforcement originally happened after a
native listen/connect/accept operation. Those entry points now check capacity
before creating a native socket or consuming a queued connection; `wrap_socket`
keeps its final invariant check. Regression coverage verifies capacity recovery
and that accepted connections remain queued while the cap is full. Native
Windows builds and tests have since passed locally with Visual Studio 2026; the
VS2022 GitHub Actions jobs remain the hosted CI authority.
