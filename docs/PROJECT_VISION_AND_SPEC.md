# LinkOS — Product Vision & Technical Specification

> **Document type:** Product charter and engineering specification  
> **Project:** LinkOS  
> **Repository:** `linkos`  
> **Current project version:** `0.1.0` (foundation)  
> **Status:** Initial specification — implementation scope and architecture baseline  
> **Primary implementation language:** C17  
> **Build system:** CMake  
> **Initial interface:** CLI only  
> **Initial runtime model:** Foreground process  
> **Initial distribution channel:** GitHub Releases  
> **Repository visibility:** Private  
> **License:** MIT

---

## 1. Executive summary

LinkOS is a cross-platform, peer-to-peer application for discovering devices on a local network, establishing authenticated connections between them, browsing explicitly shared directories, and transferring files.

The product is intended to make moving files between computers on the same network straightforward without requiring users to manually configure a server, know the other device's IP address, or use a third-party cloud storage service.

Every LinkOS installation is a peer. A device can advertise its availability, discover other LinkOS devices, initiate a connection, accept an incoming connection, and disconnect independently. The product must not be designed around a permanently designated server and client.

The first release is a native, CLI-first application for Windows and Linux. It will use one shared C codebase, with operating-system-specific functionality isolated behind platform adapters. It will be distributed as ready-to-install/downloadable builds through GitHub Releases.

The long-term objective is a dependable, secure, maintainable networking product—not merely a networking demonstration or a one-off file-transfer script.

## 2. Product vision

### 2.1 The problem

Moving files between computers on the same local network can involve unnecessary friction:

- Users may need to find and type IP addresses manually.
- Temporary file servers often require separate setup and configuration.
- Different operating systems provide different sharing mechanisms.
- Cloud-based transfer can be unnecessary when both devices are on the same LAN.
- Ad-hoc scripts are often difficult to install, secure, maintain, and use reliably.

LinkOS aims to provide one consistent product and interaction model across supported desktop operating systems.

### 2.2 The intended experience

A user installs LinkOS on two or more computers. When LinkOS is running and network discovery is enabled, each device can find other LinkOS devices on the same local network.

The user can inspect discovered devices, establish a connection, approve or authenticate the peer as required, browse the directories that peer has explicitly shared, and transfer files according to the permissions granted by the owner.

The experience should be:

- **Discoverable:** devices on the same LAN can find one another without manual IP entry in the normal case.
- **Peer-to-peer:** either device can initiate a connection.
- **Cross-platform:** commands and behavior remain consistent across Windows and Linux.
- **Secure by default:** no filesystem access or transfer permission is granted implicitly.
- **CLI-first:** all first-release features are usable from a terminal.
- **Installable:** end users receive compiled packages and do not need to clone or compile the source.
- **Maintainable:** clear modules, documented protocols, automated tests, and predictable releases.

### 2.3 Long-term direction

The first release focuses on reliable local-network discovery and file transfer. The architecture should leave room for future capabilities without implementing speculative features prematurely.

Potential future directions include:

- A graphical interface using the same core and protocol.
- Better transfer recovery, pause/resume, and integrity verification.
- Device identity management and trusted-device lists.
- More advanced network diagnostics and connection strategies.
- Optional operation beyond a single local subnet, if a secure and supportable design is justified.
- Additional operating systems, subject to testing and packaging support.

These are future possibilities, not first-release commitments.

## 3. Product principles and non-negotiable constraints

1. **One product, one shared codebase.** Windows and Linux must implement the same product behavior and wire protocol.
2. **Native platform adapters.** OS-specific APIs belong behind explicit interfaces; platform differences must not leak throughout the application.
3. **Peer symmetry.** Every installation can advertise, discover, initiate, accept, and terminate connections.
4. **No implicit trust.** Discovery does not mean authorization. A discovered device must not automatically gain access to files.
5. **Explicit filesystem boundaries.** Only user-approved shared roots may be browsed. Never expose the entire filesystem.
6. **No remote shell.** LinkOS is a file-sharing/networking product, not a remote administration tool. Do not add arbitrary command execution.
7. **No custom cryptography.** Use established, reviewed cryptographic libraries and protocols. Never invent encryption, authentication, key exchange, or signing algorithms.
8. **CLI-only first release.** Do not build a GUI, web dashboard, Electron application, or graphical configuration utility for v1.
9. **Foreground runtime first.** `linkos start` runs in the current terminal and supports graceful shutdown. Do not introduce a background daemon/service in v1 unless the product owner explicitly changes this decision.
10. **Installable releases.** End users should not need a compiler, CMake, Git, or source checkout to use a release.
11. **Test every important boundary.** Protocol parsing, path validation, permissions, connection lifecycle, and transfer behavior require automated tests.
12. **No machine-specific assumptions.** Do not hardcode development-machine paths, IP addresses, usernames, network interfaces, or ports without a documented configuration reason.

## 4. Confirmed product decisions

| Area | Decision |
|---|---|
| Product name | LinkOS |
| Repository | `linkos` |
| Repository visibility | Private |
| License | MIT |
| Initial language | C17 |
| Build system | CMake |
| Initial platforms | Windows 11 x64 and Linux x64 |
| Linux development/test target | Ubuntu 24.04 LTS x64 |
| Initial interface | CLI only |
| Runtime | Foreground process |
| Distribution | GitHub Releases |
| Product model | Peer-to-peer; no permanent server/client roles |
| Network discovery | UDP-based local-network discovery |
| Data connections | TCP-based sessions initially |
| Filesystem access | Explicitly approved shared directories only |
| GUI | Deferred; not part of v1 |

### 4.1 Important platform distinction

The product should share source code, protocol definitions, and application behavior. It is not expected to produce one identical native executable for Windows and Linux.

Separate native builds are expected:

- Windows x64 executable/package.
- Linux x64 package(s), with the initial package format selected during packaging work.

Both builds must come from the same tagged source version and pass the same applicable test suite.

## 5. Intended first release (v1)

The first release should deliver a coherent, usable local-network file-sharing workflow—not just isolated networking examples.

### 5.1 Core user workflow

1. Install LinkOS on two computers.
2. Start LinkOS in the foreground on both devices.
3. Each device advertises its presence and discovers other LinkOS devices on the local network.
4. The user lists discovered devices.
5. The user selects a device and establishes a connection.
6. The peers authenticate/pair according to the security design.
7. The user browses only the directories explicitly shared by the remote peer.
8. The user inspects file and directory metadata.
9. The user downloads an allowed file.
10. Where explicitly enabled by the receiving peer, the user can upload into a designated writable destination.
11. The user can inspect connection and transfer status and disconnect cleanly.
12. Either peer can stop LinkOS without leaving stale sessions or corrupting completed files.

### 5.2 Required v1 capabilities

#### A. CLI and application lifecycle

- A consistent `linkos` command-line executable.
- Help and version output.
- Foreground startup.
- Graceful shutdown on Ctrl+C / platform equivalent.
- Clear status and diagnostic output.
- Predictable exit codes.
- Useful error messages that explain what the user can do next.
- Configuration and log locations appropriate to each OS.

#### B. Device discovery

- Discover LinkOS peers on the local network using UDP.
- Advertise the local device while discovery/runtime is active.
- Include only the minimum information required for discovery, such as protocol compatibility, device identifier, display name, and connection endpoint.
- Handle multiple network interfaces where practical.
- Expire stale discovery entries.
- Avoid treating discovery messages as authenticated identity.
- Allow discovery to be disabled through configuration.
- Do not assume that all Wi-Fi access points permit client-to-client traffic or broadcast/multicast.

#### C. Connections and peer lifecycle

- Either peer can initiate a TCP connection.
- Either peer can accept an incoming connection while running.
- Multiple peers should be representable in the connection manager, even if the initial release limits simultaneous sessions.
- Connection states must be explicit (for example: discovered, connecting, awaiting approval, connected, disconnecting, disconnected, failed).
- Handle peer disappearance, timeouts, rejected connections, and abrupt disconnects.
- A connection must not be considered trusted merely because TCP connected.

#### D. Pairing, authentication, and transport security

- Define a clear first-contact and pairing experience.
- Users must be able to identify which device they are authorizing.
- Authenticate peers before allowing protected operations.
- Protect file-transfer sessions against passive network observation and tampering using an established secure transport.
- Store credentials or trust material using appropriate OS facilities where feasible; do not store plaintext secrets casually.
- Provide a way to revoke a trusted peer.
- Do not implement custom cryptographic primitives or protocols.
- Evaluate a maintained TLS/cryptographic library and its Windows/Linux packaging implications before implementation.
- Document the threat model and limitations, including local-network spoofing and denial-of-service considerations.

#### E. Shared directories and filesystem safety

- Users explicitly add and remove shared directories.
- A share has an explicit permission policy.
- Read-only sharing is the safe default.
- Uploading must require a separately designated writable destination or inbox; it must not imply arbitrary write access to a shared directory.
- Browsing and transfer requests must be confined to approved roots.
- Reject path traversal (`..`), absolute-path escapes, malformed paths, and paths that resolve outside the approved root.
- Handle symlinks/reparse points safely; do not allow them to escape the approved share.
- Do not expose hidden/system-sensitive locations by default.
- Do not follow a path based only on a client-supplied string without validating it against the approved root.
- Never permit remote deletion or overwrite unless a future, separately approved feature defines explicit safeguards.

#### F. File browsing and metadata

- List files and directories within an approved share.
- Return safe metadata such as name, type, size, and modification time where supported.
- Paginate or bound large directory listings.
- Handle inaccessible files, disappearing files, and permission errors gracefully.
- Avoid sending arbitrary local absolute paths to remote peers.

#### G. File transfers

- Support download from an explicitly shared remote directory.
- Support upload only to a separately approved writable destination.
- Display progress, transferred bytes, total size when known, and transfer speed where practical.
- Detect incomplete and failed transfers.
- Write to a temporary destination and finalize atomically where the platform/filesystem permits.
- Verify integrity using a standard cryptographic hash (for example SHA-256) through a maintained library.
- Prevent accidental overwrite by default; make collision behavior explicit.
- Bound memory use; transfer large files in chunks/streams rather than loading them entirely into memory.
- Handle cancellation and connection loss safely.
- Resume support is desirable, but can be deferred if it would compromise v1 reliability or schedule.

#### H. Diagnostics and configuration

- A `doctor` command for basic environment and runtime diagnostics.
- Configurable device display name and discovery behavior.
- Configurable listening port or a documented automatic-port strategy.
- Configurable share roots and permissions.
- Log levels and a log file option.
- No secrets in logs.
- Useful diagnostics for firewall, interface, bind, and connectivity failures where detectable.

### 5.3 Proposed CLI command surface

The command names below are a starting contract. Codex may refine flags and argument grammar, but should preserve a coherent, scriptable CLI and document any changes.

```text
linkos --version
linkos help
linkos doctor

linkos start
linkos status
linkos devices

linkos connect <device>
linkos connections
linkos disconnect <connection-or-device>

linkos shares
linkos share add <path> [--read-only]
linkos share remove <share-id>

linkos browse <device-or-connection> [remote-path]
linkos download <device-or-connection> <remote-path> [--to <local-path>]
linkos upload <local-path> <device-or-connection> [--to <remote-destination>]

linkos config show
linkos logs
```

The product may also offer an interactive shell (`linkos` opening a `linkos>` prompt) if this can be implemented cleanly without complicating the command interface. It is not mandatory for the first working milestone.

### 5.4 Explicit v1 exclusions

Do not implement these as part of the first release:

- GUI, desktop tray, web dashboard, or mobile application.
- Remote shell, remote desktop, or arbitrary remote command execution.
- Cloud relay or public internet account service.
- Anonymous public file sharing.
- Automatic access to home directories or whole disks.
- Remote delete or unrestricted remote file modification.
- Custom encryption or custom cryptographic algorithms.
- Complex NAT traversal before the local-network workflow is reliable.
- Background daemon/service mode.
- Plugin marketplace or scripting extension system.
- Speculative features that are not necessary for the core workflow.

## 6. Technical architecture

The implementation should be modular and testable. Keep the core portable and make operating-system differences explicit.

### 6.1 High-level component model

```text
CLI
 |
Application / Lifecycle Core
 |
+-- Configuration
+-- Logging / Diagnostics
+-- Device Discovery
+-- Connection Manager
|    |
|    +-- Platform Socket Adapter
|    +-- Secure Session Layer
|    +-- Protocol / Message Framing
|
+-- Pairing / Device Trust
+-- Shared Filesystem Service
+-- Transfer Manager
|
Platform Services
     +-- Windows adapter
     +-- Linux adapter
```

### 6.2 Suggested source layout

The existing repository is a small foundation. Extend it deliberately; do not reorganize everything without a clear reason.

```text
linkos/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── LICENSE
├── .gitignore
├── .gitattributes
├── include/
│   └── linkos/
│       ├── commands.h
│       ├── log.h
│       └── platform.h
├── src/
│   ├── main.c
│   ├── cli/
│   ├── app/
│   ├── core/
│   ├── config/
│   ├── logging/
│   ├── discovery/
│   ├── net/
│   ├── protocol/
│   ├── security/
│   ├── pairing/
│   ├── fs/
│   ├── transfer/
│   └── platform/
│       ├── windows/
│       └── linux/
├── tests/
│   ├── unit/
│   ├── integration/
│   └── fixtures/
├── docs/
│   ├── PROJECT_VISION_AND_SPEC.md
│   ├── ARCHITECTURE.md
│   ├── PROTOCOL.md
│   ├── SECURITY.md
│   ├── ROADMAP.md
│   └── DEVELOPMENT.md
└── packaging/
    ├── windows/
    └── linux/
```

This is a target structure, not a requirement to create empty directories for every future component immediately. Add modules when they have real responsibilities and code.

### 6.3 Language and portability

- Use standard C17 for shared application code.
- Prefer explicit ownership and lifecycle rules for allocated memory and resources.
- Keep public interfaces small and document ownership, error handling, and thread-safety expectations.
- Avoid compiler-specific extensions in shared code.
- Use fixed-width integer types for protocol fields and sizes.
- Validate all untrusted lengths before allocation or indexing.
- Avoid undefined behavior, unchecked arithmetic, unsafe string handling, and unbounded buffers.
- Use platform-specific implementations only where the OS requires them.

### 6.4 Platform abstraction

Windows and Linux socket APIs differ and must not be incorrectly treated as interchangeable.

- Windows: Winsock2 (`SOCKET`, `WSAStartup`, `closesocket`, `WSAGetLastError`, etc.).
- Linux: POSIX sockets (`int` descriptors, `socket`, `close`, `errno`, etc.).

Do not assume a Windows `SOCKET` is an `int`. Do not leak Winsock or POSIX error codes through the shared application layer. Define a platform-neutral handle/error abstraction and translate errors inside the platform adapter.

The same rule applies to signals/console control, filesystem metadata, path handling, secure credential storage, and packaging.

### 6.5 Networking model

Initial design:

- UDP for local-network discovery and presence announcements.
- TCP for reliable application data sessions.
- An established secure transport for authenticated, encrypted sessions.
- Explicit timeouts, bounded buffers, and robust connection teardown.

Do not hardcode one IP address, network adapter, subnet, or development port. Support configurable or safely selected ports and handle multiple interfaces as the design matures.

Discovery is a convenience mechanism, not a security boundary. Every connection must independently authenticate and authorize its peer.

### 6.6 Protocol design

The application protocol must be documented and versioned before it becomes difficult to change.

At minimum, define:

- Protocol version and compatibility behavior.
- Message type and request/response correlation identifier.
- Explicit payload length with strict maximums.
- Encoding and byte order.
- Authentication and authorization state requirements.
- Error response format.
- Timeout and cancellation semantics.
- Transfer metadata and integrity verification.
- Behavior for unknown message types and unsupported versions.

Do not serialize raw C structs directly onto the network. Struct padding, alignment, and byte order are not a portable wire format. Encode and decode fields explicitly.

Treat every network message as untrusted input. Reject malformed, oversized, truncated, unexpected, or out-of-state messages.

### 6.7 Concurrency and resource lifecycle

Choose a concurrency model deliberately. Keep it understandable and testable.

The M2 transport currently uses a single-threaded readiness loop: one thread owns
all socket handles and polls a bounded set of up to 64 sockets. Do not operate on a
socket from multiple threads. Keep event-loop work short; revisit a bounded worker
pool if file I/O, hashing, or other long-running transfer work is added.

- Avoid uncontrolled thread creation.
- Define ownership of sockets and buffers.
- Keep connect, accept, send, receive, and poll waits bounded; DNS resolution remains blocking.
- Close all socket handles before network cleanup.
- Bound the number of concurrent peers and transfers.
- Add synchronization and document lock ordering before introducing multiple network threads.
- Add tests for disconnect during discovery, connection setup, browsing, and transfer.

## 7. Security and privacy requirements

Security is a core product requirement, not a feature to bolt on after file transfer works.

### 7.1 Trust boundaries

- LAN presence announcements are untrusted.
- Device display names and metadata are untrusted.
- TCP connection establishment does not authenticate a peer.
- Remote paths and filenames are untrusted.
- File contents are untrusted.
- Configuration files may contain sensitive information and require appropriate permissions.
- Logs must not expose secrets, private keys, pairing tokens, or sensitive file contents.

### 7.2 Required protections

- Authenticated peer sessions.
- Encryption and integrity protection using established protocols.
- Explicit user consent for first-time pairing/trust.
- Revocable peer trust.
- Explicit share roots and permissions.
- Canonical path validation and traversal prevention.
- Symlink/reparse-point escape prevention.
- Bounded message sizes, directory listings, and transfer concurrency.
- Timeouts and rate/resource limits for network requests.
- Safe temporary-file and finalization behavior.
- No remote execution capability.
- No silent firewall changes or privilege escalation.

### 7.3 Firewall and network behavior

LinkOS should explain which network access is required. It must not silently disable firewalls or broadly open ports. Any installer firewall integration must be narrow, documented, and consent-based.

The product must handle networks that block peer-to-peer traffic gracefully and explain that discovery may be unavailable on guest Wi-Fi, isolated client networks, or networks that filter broadcast/multicast.

### 7.4 Security documentation

Before calling the first release production-ready, document:

- Threat model and trust assumptions.
- Pairing and authentication flow.
- Key/certificate lifecycle and revocation.
- Secure transport choice and dependency versioning.
- Filesystem sandboxing model.
- Known limitations and mitigations.

## 8. Configuration, data, and logging

### 8.1 Configuration

Configuration should be stored in a per-user application directory, not in a machine-specific development path.

Initial platform conventions:

- Windows: use an appropriate per-user application-data location (for example `%APPDATA%\LinkOS`).
- Linux: respect `$XDG_CONFIG_HOME`, falling back to `~/.config/linkos`.

Separate configuration, logs, cache, and persistent identity/trust data where appropriate. Do not assume these locations are writable without checking.

Configuration must have documented defaults, validation, and a version/migration strategy.

### 8.2 Logging

- Provide levels such as DEBUG, INFO, WARN, and ERROR.
- Support console output and an optional persistent log file.
- Include useful context such as subsystem and connection identifier.
- Never log passwords, tokens, private keys, or file contents.
- Avoid excessive logging of personal file paths unless needed and clearly documented.
- Ensure logging failure does not crash the application.

## 9. Build, test, and release engineering

### 9.1 Development platforms

Primary development starts on Windows 11 x64. Linux compatibility must be maintained and verified on Ubuntu 24.04 LTS x64 using the Lenovo development machine.

Do not defer Linux portability until the Windows implementation is complete. Each feature should be designed with both adapters in mind and tested on both platforms as early as practical.

### 9.2 Build system

- CMake is the canonical build system.
- Support a modern C17 compiler on both platforms.
- Keep dependencies explicit and pinned/documented.
- Support Debug and Release configurations.
- Keep build artifacts out of version control.
- Add reproducible build instructions.

### 9.3 Automated tests

Required test categories:

- Unit tests for CLI parsing, configuration, protocol encoding/decoding, path validation, and transfer bookkeeping.
- Integration tests for loopback TCP, discovery parsing, connection lifecycle, and file transfer.
- Cross-platform tests on Windows and Linux.
- Negative tests for malformed messages, oversized lengths, invalid paths, unauthorized operations, and interrupted transfers.
- Static analysis and compiler warnings treated seriously.
- Sanitizer runs where supported and practical.

Tests must not rely on the developer's personal IP address, username, home directory, or a particular Wi-Fi adapter.

### 9.4 CI

Use GitHub Actions to build and test on Windows and Linux for pushes and pull requests. Release workflows should produce platform-specific artifacts from version tags.

### 9.5 Distribution

GitHub Releases is the initial distribution channel.

Expected release outputs:

- Windows x64 executable and an installer/package when the packaging design is ready.
- Linux x64 package; choose `.deb` first for the Ubuntu target, with AppImage considered separately.
- Release notes, checksums, version information, and installation/uninstallation instructions.

Do not claim a package is supported until it has been tested on a clean target environment.

## 10. Development workflow and agent instructions

Codex and other coding assistants may contribute implementation, tests, documentation, and reviews. They must operate under this specification.

### 10.1 Required agent behavior

- Read this document and the existing repository documentation before making changes.
- Inspect the current code and Git status before editing.
- Do not overwrite or discard user work.
- Do not introduce a new language or framework without explicit approval.
- Do not add a GUI in v1.
- Do not introduce a background service in v1.
- Do not invent custom cryptography.
- Do not implement remote shell or unrestricted filesystem access.
- Do not silently weaken security checks to make tests pass.
- Do not add large dependencies without explaining their purpose, licensing, maintenance, and packaging impact.
- Do not claim tests passed unless they were actually run; report platform, command, and result.
- Update documentation and tests when behavior or interfaces change.
- Keep changes reviewable, with meaningful commits or logically separated implementation stages.
- If a requirement is ambiguous or a security decision is consequential, stop and document the question rather than guessing.

### 10.2 Engineering quality

- Prefer small modules with clear responsibilities.
- Keep functions focused and error handling explicit.
- Avoid global mutable state unless justified.
- Define ownership and cleanup for every resource.
- Avoid unnecessary abstraction and speculative frameworks.
- Use consistent naming and formatting.
- Keep comments useful; explain reasoning and non-obvious constraints, not every line.
- Ensure warnings are addressed rather than suppressed indiscriminately.
- Include reproducible commands in documentation.

### 10.3 Definition of done for a feature

A feature is not complete merely because the happy path works on one machine. It is complete when:

1. Its behavior and constraints are documented.
2. Errors and failure paths are handled.
3. Security and resource limits are considered.
4. Automated tests cover important normal and negative cases.
5. It builds on supported platforms or has a documented, temporary platform limitation.
6. Existing tests still pass.
7. No unrelated files or generated artifacts are committed.
8. The final report states what changed, what was tested, and what remains.

## 11. Roadmap and milestones

The milestones below define a sensible sequence. They are not permission to skip testing or merge every stage into one unreviewed change.

### M0 — Repository foundation

- [x] Private GitHub repository named `linkos`.
- [x] MIT license.
- [x] C17 and CMake foundation.
- [x] Initial CLI dispatch (`help`, `--version`, `doctor`, foreground `start`).
- [x] Basic platform identification and config-path lookup.
- [x] Initial tests and architecture notes.
- [x] Windows/MSVC build verified locally with Visual Studio 2026.
- [x] `.gitattributes` added for consistent line endings.

### M1 — Application lifecycle and developer experience

- [ ] Refine CLI command and argument parsing.
- [x] Implement foreground application lifecycle.
- [x] Graceful Ctrl+C/SIGTERM shutdown on Linux (built and tested).
- [x] Windows console shutdown implementation compiled and verified natively (control-event test).
- [ ] Configuration loading, validation, defaults, and migration version.
- [ ] Structured logging and log-file handling.
- [ ] `status`, `config`, and `doctor` behavior.
- [x] Linux Debug/Release builds and CLI checks (passed in Ubuntu 24.04 WSL).
- [x] Windows Debug/Release builds and CLI checks passed locally with VS 2026 (test harness uses CTRL_BREAK).
- [ ] Hosted Windows CI results (workflow configured; pending).

### M2 — Portable networking primitives

- [x] Define platform-neutral socket and error interfaces.
- [x] Implement and verify Winsock2 adapter (native VS 2026 builds and tests passed).
- [x] Implement POSIX adapter (built and tested on Ubuntu 24.04 via WSL).
- [x] Add bounded multi-socket readiness polling and partial-send API (Linux tests passed).
- [x] Add TCP loopback tests for simultaneous peers, bidirectional data, partial sends, close, timeout, cleanup, and capacity recovery (Linux run passed).
- [x] Run the TCP loopback suite on Windows (native VS 2026 Debug/Release).
- [x] Bounded readiness waits, shutdown/cleanup behavior, and portable error mapping.
- [x] Single-threaded `select` readiness loop, bounded to 64 open sockets.
- [x] ADR comparing event loop, worker thread, worker pool, and hybrid designs.
- [ ] UDP send/receive primitives and tests.

### M3 — Discovery and device identity

- [ ] Define and document discovery message format.
- [ ] UDP discovery and presence announcements.
- [ ] Stale peer expiry and multi-interface behavior.
- [ ] Random per-process discovery instance ID; stable authenticated identity deferred to pairing.
- [ ] `devices` CLI output and discovery diagnostics.

### M4 — Protocol and secure peer sessions

- [ ] Versioned application framing.
- [ ] Request/response correlation and bounded decoding.
- [ ] Select and integrate maintained secure transport.
- [ ] Pairing, authentication, trust storage, and revocation.
- [ ] Negative security and protocol tests.

### M5 — Shared directories and browsing

- [ ] Share management and permission model.
- [ ] Safe path resolution and symlink/reparse-point protections.
- [ ] Remote directory listing and metadata.
- [ ] Authorization tests for every filesystem operation.

### M6 — File transfer

- [ ] Chunked download.
- [ ] Explicit writable upload destination.
- [ ] Progress, cancellation, and error reporting.
- [ ] Temporary-file finalization and collision handling.
- [ ] Integrity verification.
- [ ] Interrupted-transfer tests; evaluate resume support.

### M7 — Release readiness

- [ ] Windows package/installer.
- [ ] Linux `.deb` package.
- [ ] Clean-machine installation tests.
- [ ] GitHub Actions release workflow.
- [ ] Versioned release notes and checksums.
- [ ] User documentation and security review.
- [ ] End-to-end test across Windows and Ubuntu on a real LAN.

## 12. Initial acceptance criteria for v1

The first release can be considered functionally ready only when all of the following are demonstrated:

1. A user can install LinkOS on supported Windows and Linux systems without compiling source.
2. Both installations can start and stop in the foreground reliably.
3. Two devices on a compatible LAN can discover one another without manually entering IP addresses.
4. Either device can initiate a connection to the other.
5. A user can authenticate/pair and revoke trust.
6. An untrusted peer cannot browse or transfer files.
7. A trusted peer can browse only explicitly shared directories.
8. Path traversal and symlink/reparse-point escape attempts are rejected.
9. A user can download a file and verify its integrity.
10. Upload works only when the receiving user has explicitly configured a writable destination.
11. Large transfers do not require loading the entire file into memory.
12. Interrupted and failed transfers do not silently appear successful or leave corrupt final files.
13. Both platforms pass the relevant automated tests.
14. Logs and diagnostics are useful without exposing secrets.
15. The release includes installation, usage, security, and troubleshooting documentation.

## 13. Current repository baseline

The current repository contains:

- `CMakeLists.txt` and `CMakePresets.json`
- `src/main.c`
- `src/cli/commands.c`
- `src/core/log.c`
- `src/platform/platform.c`
- `src/net/socket.c` and `include/linkos/net.h` for the internal TCP socket API
- public headers under `include/linkos/`
- `tests/test_cli.c`
- `tests/test_net.c` and `tests/test_cli_integration.py`
- `docs/PROJECT_VISION_AND_SPEC.md`, `docs/PROTOCOL.md`, and `docs/SECURITY.md`
- `docs/ARCHITECTURE.md`
- `docs/ROADMAP.md`
- `README.md`
- `LICENSE`

Current CLI behavior:

- `linkos help` prints basic usage.
- `linkos --version` prints the project version.
- `linkos doctor` prints basic platform/config-path information.
- `linkos start` runs in the foreground and handles shutdown signals; it does not open a network listener.

The TCP library is exercised by local loopback tests, but it is not yet wired into the CLI. Discovery, application protocol, secure sessions, and file sharing are not implemented. The project is not yet a functioning network file-sharing application.

## 14. Open technical decisions to resolve during design

These questions require investigation and documented decisions before the relevant feature is implemented:

- Which maintained TLS/secure transport library best fits C17, Windows/Linux support, packaging, and license requirements?
- How should device identity and pairing work without requiring a cloud account?
- What exact discovery transport and message format best handle Windows/Linux firewalls and common LAN configurations?
- How should the app select/listen on ports and advertise endpoints across multiple interfaces?
- What concurrency model best balances simplicity, responsiveness, and portability?
- Which configuration format and parser should be used?
- What is the minimum supported Windows version and Linux distribution baseline for v1?
- Which Linux package formats should be officially supported at launch?
- Should interactive shell mode be part of v1 or remain optional?
- What transfer-resume behavior is worth supporting in the first release?

For each decision, record the options considered, rationale, security implications, dependency/licensing implications, and the selected approach in the relevant architecture document or an ADR (Architecture Decision Record).

## 15. Product success criteria

LinkOS is succeeding when a user can install it on two supported computers, run it from the terminal, discover and securely connect to the other device, browse only the files that device has intentionally shared, and transfer files reliably—without needing cloud storage, manual network setup in the normal case, or a developer toolchain.

The quality bar is not merely “it works on the developer's Wi-Fi.” It must be understandable, secure by default, diagnosable, tested on both operating systems, and maintainable by contributors who did not write the first version.

---

**End of specification.**
