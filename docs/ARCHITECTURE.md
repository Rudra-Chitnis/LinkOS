# LinkOS architecture notes

## Current foundation

The executable is deliberately small. `src/main.c` delegates to the CLI command dispatcher. The command layer uses shared core services and a platform adapter. No socket API is exposed to the CLI.

## Upcoming networking boundary

The next networking milestone should introduce an opaque socket handle and a platform API, rather than assuming POSIX file-descriptor semantics:

- Windows adapter: Winsock2 (`SOCKET`, `WSAStartup`, `closesocket`, `WSAGetLastError`)
- Linux adapter: POSIX sockets (`int`, `socket`, `close`, `errno`)
- Shared networking layer: listen/connect/send/receive abstractions and explicit error values

Do not typedef a Windows `SOCKET` as `int`. Keep startup/shutdown and error translation inside the platform adapter.

## Security constraints

- Pair devices explicitly before allowing file access.
- Share only user-selected directories.
- Canonicalize requested paths and prevent traversal outside approved roots.
- Do not follow symlinks outside a shared root.
- No remote shell or arbitrary command execution.
- Design protocol framing with bounded lengths, version negotiation, and request IDs.
