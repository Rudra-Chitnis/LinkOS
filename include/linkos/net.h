#ifndef LINKOS_NET_H
#define LINKOS_NET_H

#include <stddef.h>
#include <stdint.h>

typedef struct linkos_net_socket linkos_net_socket_t;

#define LINKOS_NET_MAX_SOCKETS 64U
#define LINKOS_NET_EVENT_READ  0x01U
#define LINKOS_NET_EVENT_WRITE 0x02U

typedef enum {
    LINKOS_NET_OK = 0,
    LINKOS_NET_INVALID_ARGUMENT,
    LINKOS_NET_NOT_INITIALIZED,
    LINKOS_NET_OUT_OF_MEMORY,
    LINKOS_NET_RESOLVE_FAILED,
    LINKOS_NET_SOCKET_FAILED,
    LINKOS_NET_BIND_FAILED,
    LINKOS_NET_LISTEN_FAILED,
    LINKOS_NET_CONNECT_FAILED,
    LINKOS_NET_ACCEPT_FAILED,
    LINKOS_NET_TIMEOUT,
    LINKOS_NET_CLOSED,
    LINKOS_NET_IO_FAILED,
    LINKOS_NET_BUSY,
    LINKOS_NET_WOULD_BLOCK,
    LINKOS_NET_RESOURCE_LIMIT
} linkos_net_error_t;

/*
 * Single-thread event-loop API. All calls and socket ownership must stay on one
 * thread; do not close or operate on a handle concurrently. At most 64 sockets
 * may be open at once. Initialization is idempotent; cleanup is BUSY while any
 * socket is open.
 */
/* Initializes Winsock on Windows; no-op initialization on Linux. Idempotent. */
linkos_net_error_t linkos_net_initialize(void);
/* Returns BUSY while any socket wrapper is open; otherwise shuts down the backend. */
linkos_net_error_t linkos_net_cleanup(void);
/* Returns a static, human-readable string for every error enum value. */
const char *linkos_net_error_string(linkos_net_error_t error);

/*
 * Creates a nonblocking TCP listener. host==NULL binds all interfaces; port==0
 * requests an ephemeral port. On success, caller owns *out_socket and must close
 * it. Failure leaves *out_socket NULL. The resource cap counts this listener.
 */
linkos_net_error_t linkos_net_listen(const char *host, uint16_t port,
                                     int backlog, linkos_net_socket_t **out_socket);
/*
 * Resolves host synchronously, then attempts TCP connection within timeout_ms
 * across resolved addresses. DNS time is excluded. Failure leaves output NULL.
 */
linkos_net_error_t linkos_net_connect(const char *host, uint16_t port,
                                      unsigned int timeout_ms,
                                      linkos_net_socket_t **out_socket);
/* Accepts one queued connection within timeout_ms; output is NULL on failure.
 * Returns RESOURCE_LIMIT before waiting/accepting if the 64-handle cap is full. */
linkos_net_error_t linkos_net_accept(linkos_net_socket_t *listener,
                                     unsigned int timeout_ms,
                                     linkos_net_socket_t **out_socket);

/*
 * Waits for read/write readiness for a bounded array. interests and ready contain
 * count byte flags using LINKOS_NET_EVENT_*; ready is zeroed on entry. Returns OK
 * when any requested event is ready, TIMEOUT when none is ready before timeout_ms,
 * or an error. Readiness is advisory: subsequent I/O can still return WOULD_BLOCK
 * or an error. A readable socket may have data or may have been closed by its peer.
 */
linkos_net_error_t linkos_net_poll(linkos_net_socket_t *const sockets[],
                                   const uint8_t interests[], uint8_t ready[],
                                   size_t count, unsigned int timeout_ms);

/*
 * Performs one nonblocking send attempt of at most 64 KiB. sent is the actual
 * count, which can be smaller than length; WOULD_BLOCK means sent is zero.
 */
linkos_net_error_t linkos_net_send_some(linkos_net_socket_t *socket,
                                        const void *data, size_t length,
                                        size_t *sent);
/*
 * Sends the entire buffer or returns a timeout/error. Partial sends are retried;
 * timeout_ms is the maximum duration for the operation (DNS resolution excluded).
 * This can occupy the event-loop thread until completion or timeout; use poll and
 * send_some when responsiveness to other peers matters. sent reports progress on
 * success and on error; OK is returned only when sent == length.
 */
linkos_net_error_t linkos_net_send_all(linkos_net_socket_t *socket,
                                       const void *data, size_t length,
                                       size_t *sent,
                                       unsigned int timeout_ms);
/*
 * Waits up to timeout_ms for data; returns 1..capacity bytes or timeout/closed/error.
 * received is set to zero on entry and reports the actual byte count on success.
 */
linkos_net_error_t linkos_net_receive(linkos_net_socket_t *socket,
                                      void *buffer, size_t capacity,
                                      size_t *received, unsigned int timeout_ms);
/* Returns the bound local TCP port in host byte order. */
linkos_net_error_t linkos_net_local_port(linkos_net_socket_t *socket,
                                         uint16_t *out_port);
/*
 * Closes and releases the socket. POSIX clears the handle even if close reports
 * an error; Windows preserves it on closesocket failure so the caller can retry.
 * NULL handles are accepted.
 */
linkos_net_error_t linkos_net_close(linkos_net_socket_t **socket);

#endif
