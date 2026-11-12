# Security status and threat model

## Current implementation

The current foundation has no network listener, peer discovery, pairing, filesystem
sharing, or transfer functionality. It therefore does not yet provide secure peer
communication and must not be presented as a file-sharing product.

The internal TCP API is a raw transport primitive with no encryption, identity
verification, or authorization. It is not wired into the CLI and must not be used
as a secure peer session.

## Intended trust boundaries

- LAN announcements and device metadata will be untrusted input.
- Proposed UDP discovery is a presence hint only: it must not establish trust,
  authorize file access, or carry secrets, filesystem paths, hostnames, or other
  unnecessary machine details. The source IP is routing metadata, never identity.
- Discovery identifiers should be random per running process (an instance ID),
  not a persistent device identity. A stable identity, if needed, is exchanged
  only during a later authenticated pairing flow. Unauthenticated discovery
  remains spoofable and replayable and must be treated accordingly.
- A discovered or connected device will not be trusted automatically.
- Browsing and transfers will require explicit peer trust and share authorization.
- Shared roots will be read-only unless a separate writable destination is approved.
- Remote paths, names, and file contents will be treated as untrusted.
- No remote command execution or unrestricted filesystem access is in scope.

Before secure sessions are implemented, an established transport and explicit
pairing/key lifecycle will be selected and documented. Custom cryptography is out
of scope. Current logs contain only fixed lifecycle messages and do not log secrets
or file contents.
