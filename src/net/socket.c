#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "linkos/net.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
typedef SOCKET native_socket_t;
typedef int socket_length_t;
#define INVALID_NATIVE_SOCKET INVALID_SOCKET
#define SEND_CHUNK_MAX (64U * 1024U)
#else
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
typedef int native_socket_t;
typedef socklen_t socket_length_t;
#define INVALID_NATIVE_SOCKET (-1)
#define SEND_CHUNK_MAX (64U * 1024U)
#endif

struct linkos_net_socket {
    native_socket_t native;
};

static int network_initialized = 0;
static size_t open_socket_count = 0U;

static int socket_capacity_available(void)
{
    return open_socket_count < (size_t)LINKOS_NET_MAX_SOCKETS;
}

static uint64_t monotonic_milliseconds(void)
{
#ifdef _WIN32
    return (uint64_t)GetTickCount64();
#else
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return 0U;
    }
    return (uint64_t)now.tv_sec * UINT64_C(1000) +
           (uint64_t)now.tv_nsec / UINT64_C(1000000);
#endif
}

static uint64_t deadline_after(unsigned int timeout_ms)
{
    return monotonic_milliseconds() + (uint64_t)timeout_ms;
}

static unsigned int remaining_timeout(uint64_t deadline)
{
    uint64_t now = monotonic_milliseconds();
    uint64_t remaining = deadline > now ? deadline - now : 0U;
    return remaining > (uint64_t)UINT_MAX ? UINT_MAX : (unsigned int)remaining;
}

static void native_close(native_socket_t socket)
{
#ifdef _WIN32
    (void)closesocket(socket);
#else
    (void)close(socket);
#endif
}

static int native_set_nonblocking(native_socket_t socket)
{
#ifdef _WIN32
    u_long enabled = 1UL;
    return ioctlsocket(socket, FIONBIO, &enabled) == 0;
#else
    int flags = fcntl(socket, F_GETFL, 0);
    int descriptor_flags;
    if (socket < 0 || socket >= FD_SETSIZE || flags < 0 ||
        fcntl(socket, F_SETFL, flags | O_NONBLOCK) != 0) {
        return 0;
    }
    descriptor_flags = fcntl(socket, F_GETFD, 0);
    return descriptor_flags >= 0 &&
           fcntl(socket, F_SETFD, descriptor_flags | FD_CLOEXEC) == 0;
#endif
}

static int native_would_block(void)
{
#ifdef _WIN32
    int error = WSAGetLastError();
    return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS ||
           error == WSAEALREADY || error == WSAEINTR;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS ||
           errno == EALREADY || errno == EINTR;
#endif
}

static int native_interrupted(void)
{
#ifdef _WIN32
    return WSAGetLastError() == WSAEINTR;
#else
    return errno == EINTR;
#endif
}

static linkos_net_error_t poll_native(const native_socket_t sockets[],
                                      const uint8_t interests[], uint8_t ready[],
                                      size_t count, uint64_t deadline)
{
    for (;;) {
        fd_set read_set;
        fd_set write_set;
        struct timeval timeout;
#ifndef _WIN32
        int maximum = -1;
#endif
        int result;
        size_t index;
        int has_read = 0;
        int has_write = 0;

        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        memset(ready, 0, count * sizeof(*ready));
        for (index = 0U; index < count; index++) {
#ifndef _WIN32
            if (sockets[index] < 0 || sockets[index] >= FD_SETSIZE) {
                return LINKOS_NET_RESOURCE_LIMIT;
            }
            if (sockets[index] > maximum) {
                maximum = sockets[index];
            }
#endif
            if ((interests[index] & LINKOS_NET_EVENT_READ) != 0U) {
                FD_SET(sockets[index], &read_set);
                has_read = 1;
            }
            if ((interests[index] & LINKOS_NET_EVENT_WRITE) != 0U) {
                FD_SET(sockets[index], &write_set);
                has_write = 1;
            }
        }
        {
            unsigned int left = remaining_timeout(deadline);
            unsigned long seconds = (unsigned long)(left / 1000U);
            timeout.tv_sec = seconds > (unsigned long)LONG_MAX ? LONG_MAX : (long)seconds;
            timeout.tv_usec = (long)((left % 1000U) * 1000U);
        }
#ifdef _WIN32
        result = select(0, has_read ? &read_set : NULL,
                        has_write ? &write_set : NULL, NULL, &timeout);
#else
        result = select(maximum + 1, has_read ? &read_set : NULL,
                        has_write ? &write_set : NULL, NULL, &timeout);
#endif
        if (result > 0) {
            for (index = 0U; index < count; index++) {
                if ((interests[index] & LINKOS_NET_EVENT_READ) != 0U &&
                    FD_ISSET(sockets[index], &read_set)) {
                    ready[index] |= LINKOS_NET_EVENT_READ;
                }
                if ((interests[index] & LINKOS_NET_EVENT_WRITE) != 0U &&
                    FD_ISSET(sockets[index], &write_set)) {
                    ready[index] |= LINKOS_NET_EVENT_WRITE;
                }
            }
            return LINKOS_NET_OK;
        }
        if (result == 0) {
            return LINKOS_NET_TIMEOUT;
        }
        if (!native_interrupted() || remaining_timeout(deadline) == 0U) {
            return LINKOS_NET_IO_FAILED;
        }
    }
}

static linkos_net_error_t wait_ready(native_socket_t socket, int writable,
                                     unsigned int timeout_ms)
{
    native_socket_t sockets[1];
    uint8_t interests[1];
    uint8_t ready[1];
    sockets[0] = socket;
    interests[0] = writable ? LINKOS_NET_EVENT_WRITE : LINKOS_NET_EVENT_READ;
    return poll_native(sockets, interests, ready, 1U, deadline_after(timeout_ms));
}

static linkos_net_error_t wrap_socket(native_socket_t native,
                                      linkos_net_socket_t **out_socket)
{
    linkos_net_socket_t *socket;
    if (open_socket_count >= (size_t)LINKOS_NET_MAX_SOCKETS) {
        native_close(native);
        return LINKOS_NET_RESOURCE_LIMIT;
    }
    socket = (linkos_net_socket_t *)malloc(sizeof(*socket));
    if (socket == NULL) {
        native_close(native);
        return LINKOS_NET_OUT_OF_MEMORY;
    }
    socket->native = native;
    *out_socket = socket;
    open_socket_count++;
    return LINKOS_NET_OK;
}

static native_socket_t create_native_socket(const struct addrinfo *address)
{
    native_socket_t native = socket(address->ai_family, address->ai_socktype,
                                    address->ai_protocol);
    if (native == INVALID_NATIVE_SOCKET) {
        return INVALID_NATIVE_SOCKET;
    }
    if (!native_set_nonblocking(native)) {
        native_close(native);
        return INVALID_NATIVE_SOCKET;
    }
    return native;
}

linkos_net_error_t linkos_net_initialize(void)
{
    if (network_initialized) {
        return LINKOS_NET_OK;
    }
#ifdef _WIN32
    {
        WSADATA data;
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            return LINKOS_NET_SOCKET_FAILED;
        }
    }
#endif
    network_initialized = 1;
    return LINKOS_NET_OK;
}

linkos_net_error_t linkos_net_cleanup(void)
{
    if (!network_initialized) {
        return LINKOS_NET_OK;
    }
    if (open_socket_count != 0U) {
        return LINKOS_NET_BUSY;
    }
#ifdef _WIN32
    if (WSACleanup() != 0) {
        return LINKOS_NET_SOCKET_FAILED;
    }
#endif
    network_initialized = 0;
    return LINKOS_NET_OK;
}

const char *linkos_net_error_string(linkos_net_error_t error)
{
    static const char *const names[] = {
        "success", "invalid argument", "network is not initialized",
        "out of memory", "address resolution failed", "socket operation failed",
        "bind failed", "listen failed", "connect failed", "accept failed",
        "operation timed out", "connection closed", "I/O operation failed",
        "network cleanup blocked by open sockets", "socket is not ready",
        "socket resource limit reached"
    };
    if ((unsigned int)error >= (unsigned int)(sizeof(names) / sizeof(names[0]))) {
        return "unknown network error";
    }
    return names[(unsigned int)error];
}

linkos_net_error_t linkos_net_poll(linkos_net_socket_t *const sockets[],
                                   const uint8_t interests[], uint8_t ready[],
                                   size_t count, unsigned int timeout_ms)
{
    native_socket_t native_sockets[LINKOS_NET_MAX_SOCKETS];
    size_t index;
    if (ready == NULL || count == 0U || count > (size_t)LINKOS_NET_MAX_SOCKETS) {
        return LINKOS_NET_INVALID_ARGUMENT;
    }
    memset(ready, 0, count * sizeof(*ready));
    if (sockets == NULL || interests == NULL) {
        return LINKOS_NET_INVALID_ARGUMENT;
    }
    if (!network_initialized) {
        return LINKOS_NET_NOT_INITIALIZED;
    }
    for (index = 0U; index < count; index++) {
        if (sockets[index] == NULL || interests[index] == 0U ||
            (interests[index] & (uint8_t)0xFCU) != 0U) {
            return LINKOS_NET_INVALID_ARGUMENT;
        }
        native_sockets[index] = sockets[index]->native;
    }
    return poll_native(native_sockets, interests, ready, count,
                       deadline_after(timeout_ms));
}

linkos_net_error_t linkos_net_listen(const char *host, uint16_t port,
                                     int backlog, linkos_net_socket_t **out_socket)
{
    struct addrinfo hints;
    struct addrinfo *addresses = NULL;
    struct addrinfo *address;
    char service[6];
    linkos_net_error_t result = LINKOS_NET_BIND_FAILED;

    if (out_socket == NULL || backlog <= 0) {
        return LINKOS_NET_INVALID_ARGUMENT;
    }
    *out_socket = NULL;
    if (!network_initialized) {
        return LINKOS_NET_NOT_INITIALIZED;
    }
    if (!socket_capacity_available()) {
        return LINKOS_NET_RESOURCE_LIMIT;
    }
    (void)snprintf(service, sizeof(service), "%u", (unsigned int)port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(host, service, &hints, &addresses) != 0) {
        return LINKOS_NET_RESOLVE_FAILED;
    }
    for (address = addresses; address != NULL; address = address->ai_next) {
        native_socket_t native = create_native_socket(address);
        if (native == INVALID_NATIVE_SOCKET) {
            result = LINKOS_NET_SOCKET_FAILED;
            continue;
        }
#ifndef _WIN32
        {
            int reuse = 1;
            (void)setsockopt(native, SOL_SOCKET, SO_REUSEADDR, &reuse,
                             (socket_length_t)sizeof(reuse));
        }
#endif
        if (bind(native, address->ai_addr, (socket_length_t)address->ai_addrlen) != 0) {
            native_close(native);
            result = LINKOS_NET_BIND_FAILED;
            continue;
        }
        if (listen(native, backlog) != 0) {
            native_close(native);
            result = LINKOS_NET_LISTEN_FAILED;
            continue;
        }
        result = wrap_socket(native, out_socket);
        if (result == LINKOS_NET_OK) {
            break;
        }
    }
    freeaddrinfo(addresses);
    return result;
}

linkos_net_error_t linkos_net_connect(const char *host, uint16_t port,
                                      unsigned int timeout_ms,
                                      linkos_net_socket_t **out_socket)
{
    struct addrinfo hints;
    struct addrinfo *addresses = NULL;
    struct addrinfo *address;
    char service[6];
    linkos_net_error_t result = LINKOS_NET_CONNECT_FAILED;
    uint64_t deadline;

    if (host == NULL || host[0] == '\0' || out_socket == NULL) {
        return LINKOS_NET_INVALID_ARGUMENT;
    }
    *out_socket = NULL;
    if (!network_initialized) {
        return LINKOS_NET_NOT_INITIALIZED;
    }
    if (!socket_capacity_available()) {
        return LINKOS_NET_RESOURCE_LIMIT;
    }
    (void)snprintf(service, sizeof(service), "%u", (unsigned int)port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(host, service, &hints, &addresses) != 0) {
        return LINKOS_NET_RESOLVE_FAILED;
    }
    deadline = deadline_after(timeout_ms);
    for (address = addresses; address != NULL; address = address->ai_next) {
        native_socket_t native = create_native_socket(address);
        if (native == INVALID_NATIVE_SOCKET) {
            result = LINKOS_NET_SOCKET_FAILED;
            continue;
        }
        if (connect(native, address->ai_addr, (socket_length_t)address->ai_addrlen) == 0) {
            result = wrap_socket(native, out_socket);
            if (result == LINKOS_NET_OK) {
                break;
            }
            continue;
        }
        if (native_would_block()) {
            result = wait_ready(native, 1, remaining_timeout(deadline));
            if (result == LINKOS_NET_OK) {
                int socket_error = 0;
                socket_length_t option_length = (socket_length_t)sizeof(socket_error);
                if (getsockopt(native, SOL_SOCKET, SO_ERROR,
#ifdef _WIN32
                               (char *)&socket_error,
#else
                               &socket_error,
#endif
                               &option_length) == 0 && socket_error == 0) {
                    result = wrap_socket(native, out_socket);
                    if (result == LINKOS_NET_OK) {
                        break;
                    }
                } else {
                    result = LINKOS_NET_CONNECT_FAILED;
                }
            }
        } else {
            result = LINKOS_NET_CONNECT_FAILED;
        }
        native_close(native);
        if (result == LINKOS_NET_OUT_OF_MEMORY || result == LINKOS_NET_TIMEOUT) {
            break;
        }
    }
    freeaddrinfo(addresses);
    return result;
}

linkos_net_error_t linkos_net_accept(linkos_net_socket_t *listener,
                                     unsigned int timeout_ms,
                                     linkos_net_socket_t **out_socket)
{
    native_socket_t native;
    linkos_net_error_t result;
    if (listener == NULL || out_socket == NULL) {
        return LINKOS_NET_INVALID_ARGUMENT;
    }
    *out_socket = NULL;
    if (!network_initialized) {
        return LINKOS_NET_NOT_INITIALIZED;
    }
    if (!socket_capacity_available()) {
        return LINKOS_NET_RESOURCE_LIMIT;
    }
    result = wait_ready(listener->native, 0, timeout_ms);
    if (result != LINKOS_NET_OK) {
        return result;
    }
    native = accept(listener->native, NULL, NULL);
    if (native == INVALID_NATIVE_SOCKET) {
        return native_would_block() ? LINKOS_NET_TIMEOUT : LINKOS_NET_ACCEPT_FAILED;
    }
    if (!native_set_nonblocking(native)) {
        native_close(native);
        return LINKOS_NET_SOCKET_FAILED;
    }
    return wrap_socket(native, out_socket);
}

linkos_net_error_t linkos_net_send_all(linkos_net_socket_t *socket,
                                       const void *data, size_t length,
                                       size_t *sent,
                                       unsigned int timeout_ms)
{
    const char *bytes = (const char *)data;
    size_t offset = 0U;
    uint64_t deadline;
    if (sent == NULL) {
        return LINKOS_NET_INVALID_ARGUMENT;
    }
    *sent = 0U;
    if (socket == NULL || (data == NULL && length != 0U)) {
        return LINKOS_NET_INVALID_ARGUMENT;
    }
    if (!network_initialized) {
        return LINKOS_NET_NOT_INITIALIZED;
    }
    deadline = deadline_after(timeout_ms);
    while (offset < length) {
        size_t chunk_sent = 0U;
        linkos_net_error_t result;
        if (offset != 0U && remaining_timeout(deadline) == 0U) {
            return LINKOS_NET_TIMEOUT;
        }
        result = linkos_net_send_some(socket, bytes + offset, length - offset,
                                      &chunk_sent);
        if (result == LINKOS_NET_OK) {
            offset += chunk_sent;
            *sent = offset;
            continue;
        }
        if (result != LINKOS_NET_WOULD_BLOCK) {
            return result;
        }
        if (remaining_timeout(deadline) == 0U) {
            return LINKOS_NET_TIMEOUT;
        }
        result = wait_ready(socket->native, 1, remaining_timeout(deadline));
        if (result != LINKOS_NET_OK) {
            return result;
        }
    }
    return LINKOS_NET_OK;
}

linkos_net_error_t linkos_net_send_some(linkos_net_socket_t *socket,
                                        const void *data, size_t length,
                                        size_t *sent)
{
    int amount;
    if (sent == NULL) {
        return LINKOS_NET_INVALID_ARGUMENT;
    }
    *sent = 0U;
    if (socket == NULL || (data == NULL && length != 0U)) {
        return LINKOS_NET_INVALID_ARGUMENT;
    }
    if (!network_initialized) {
        return LINKOS_NET_NOT_INITIALIZED;
    }
    if (length == 0U) {
        return LINKOS_NET_OK;
    }
    if (length > (size_t)INT_MAX) {
        length = (size_t)INT_MAX;
    }
    if (length > (size_t)SEND_CHUNK_MAX) {
        length = (size_t)SEND_CHUNK_MAX;
    }
#ifdef _WIN32
    amount = send(socket->native, (const char *)data, (int)length, 0);
#else
    amount = (int)send(socket->native, data, length, MSG_NOSIGNAL);
#endif
    if (amount > 0) {
        *sent = (size_t)amount;
        return LINKOS_NET_OK;
    }
    if (amount == 0) {
        return LINKOS_NET_CLOSED;
    }
    return native_would_block() ? LINKOS_NET_WOULD_BLOCK : LINKOS_NET_IO_FAILED;
}

linkos_net_error_t linkos_net_receive(linkos_net_socket_t *socket,
                                      void *buffer, size_t capacity,
                                      size_t *received, unsigned int timeout_ms)
{
    int amount;
    linkos_net_error_t result;
    uint64_t deadline;
    if (received == NULL) {
        return LINKOS_NET_INVALID_ARGUMENT;
    }
    *received = 0U;
    if (socket == NULL || buffer == NULL || capacity == 0U) {
        return LINKOS_NET_INVALID_ARGUMENT;
    }
    if (!network_initialized) {
        return LINKOS_NET_NOT_INITIALIZED;
    }
    deadline = deadline_after(timeout_ms);
    if (capacity > (size_t)INT_MAX) {
        capacity = (size_t)INT_MAX;
    }
    for (;;) {
        result = wait_ready(socket->native, 0, remaining_timeout(deadline));
        if (result != LINKOS_NET_OK) {
            return result;
        }
#ifdef _WIN32
        amount = recv(socket->native, (char *)buffer, (int)capacity, 0);
#else
        amount = (int)recv(socket->native, buffer, capacity, 0);
#endif
        if (amount > 0) {
            *received = (size_t)amount;
            return LINKOS_NET_OK;
        }
        if (amount == 0) {
            return LINKOS_NET_CLOSED;
        }
        if (!native_would_block()) {
            return LINKOS_NET_IO_FAILED;
        }
        if (remaining_timeout(deadline) == 0U) {
            return LINKOS_NET_TIMEOUT;
        }
    }
}

linkos_net_error_t linkos_net_local_port(linkos_net_socket_t *socket,
                                         uint16_t *out_port)
{
    struct sockaddr_storage address;
    socket_length_t length = (socket_length_t)sizeof(address);
    if (socket == NULL || out_port == NULL) {
        return LINKOS_NET_INVALID_ARGUMENT;
    }
    if (getsockname(socket->native, (struct sockaddr *)&address, &length) != 0) {
        return LINKOS_NET_IO_FAILED;
    }
    if (address.ss_family == AF_INET) {
        *out_port = ntohs(((struct sockaddr_in *)&address)->sin_port);
    } else if (address.ss_family == AF_INET6) {
        *out_port = ntohs(((struct sockaddr_in6 *)&address)->sin6_port);
    } else {
        return LINKOS_NET_IO_FAILED;
    }
    return LINKOS_NET_OK;
}

linkos_net_error_t linkos_net_close(linkos_net_socket_t **socket)
{
    int result;
    if (socket == NULL || *socket == NULL) {
        return LINKOS_NET_OK;
    }
#ifdef _WIN32
    result = closesocket((*socket)->native);
    if (result != 0) {
        return LINKOS_NET_IO_FAILED;
    }
#else
    result = close((*socket)->native);
#endif
    free(*socket);
    *socket = NULL;
    if (open_socket_count > 0U) {
        open_socket_count--;
    }
    return result == 0 ? LINKOS_NET_OK : LINKOS_NET_IO_FAILED;
}
