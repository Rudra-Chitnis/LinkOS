#include "linkos/net.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", name);
        failures++;
    } else {
        printf("PASS: %s\n", name);
    }
}

static int receive_exact(linkos_net_socket_t *socket, char *buffer,
                         size_t expected_length)
{
    size_t total = 0U;
    while (total < expected_length) {
        size_t received = 0U;
        if (linkos_net_receive(socket, buffer + total, expected_length - total,
                               &received, 2000U) != LINKOS_NET_OK || received == 0U) {
            return 0;
        }
        total += received;
    }
    return 1;
}

static void test_connection_cycle(linkos_net_socket_t *listener, uint16_t port,
                                  unsigned int cycle)
{
    linkos_net_socket_t *client = NULL;
    linkos_net_socket_t *server = NULL;
    char outgoing[32];
    char response[32];
    size_t received = 0U;
    size_t sent = 0U;
    int outgoing_length = snprintf(outgoing, sizeof(outgoing), "client-%u", cycle);
    int response_length = snprintf(response, sizeof(response), "server-%u", cycle);
    char incoming[32];
    char returned[32];

    check(outgoing_length > 0 && response_length > 0, "prepare test payloads");
    check(linkos_net_connect("127.0.0.1", port, 2000U, &client) == LINKOS_NET_OK,
          "connect to loopback listener");
    check(linkos_net_accept(listener, 2000U, &server) == LINKOS_NET_OK,
          "accept loopback client");
    if (client == NULL || server == NULL) {
        (void)linkos_net_close(&client);
        (void)linkos_net_close(&server);
        return;
    }

    check(linkos_net_send_all(client, outgoing, (size_t)outgoing_length,
                              &sent, 2000U) == LINKOS_NET_OK &&
              sent == (size_t)outgoing_length,
          "client sends data");
    check(receive_exact(server, incoming, (size_t)outgoing_length) &&
              memcmp(incoming, outgoing, (size_t)outgoing_length) == 0,
          "server receives client data");

    check(linkos_net_send_all(server, response, (size_t)response_length,
                              &sent, 2000U) == LINKOS_NET_OK &&
              sent == (size_t)response_length,
          "server sends response");
    check(receive_exact(client, returned, (size_t)response_length) &&
              memcmp(returned, response, (size_t)response_length) == 0,
          "client receives server data");

    check(linkos_net_close(&client) == LINKOS_NET_OK && client == NULL,
          "client closes cleanly");
    received = 99U;
    check(linkos_net_receive(server, incoming, sizeof(incoming), &received, 2000U) ==
              LINKOS_NET_CLOSED && received == 0U,
          "server observes orderly client closure");
    check(linkos_net_close(&server) == LINKOS_NET_OK && server == NULL,
          "server closes cleanly");
}

static void test_simultaneous_connections(linkos_net_socket_t *listener,
                                          uint16_t port)
{
    enum { PEER_COUNT = 3, LARGE_PAYLOAD_SIZE = 128 * 1024 };
    linkos_net_socket_t *clients[PEER_COUNT] = {NULL, NULL, NULL};
    linkos_net_socket_t *servers[PEER_COUNT] = {NULL, NULL, NULL};
    uint8_t interests[PEER_COUNT];
    uint8_t ready[PEER_COUNT];
    uint8_t observed[PEER_COUNT] = {0U, 0U, 0U};
    char sent_messages[PEER_COUNT][24];
    char received_messages[24];
    char large_outgoing[LARGE_PAYLOAD_SIZE];
    char large_incoming[LARGE_PAYLOAD_SIZE];
    size_t index;
    size_t received_after_close = 0U;
    size_t first_send = 0U;
    size_t first_receive = 0U;
    size_t sent = 0U;
    linkos_net_error_t send_result;

    memset(interests, LINKOS_NET_EVENT_READ, sizeof(interests));
    for (index = 0U; index < PEER_COUNT; index++) {
        int length;
        check(linkos_net_connect("127.0.0.1", port, 2000U, &clients[index]) ==
                  LINKOS_NET_OK,
              "open a simultaneous client connection");
        length = snprintf(sent_messages[index], sizeof(sent_messages[index]),
                          "peer-%u-request", (unsigned int)index);
        check(length > 0 && linkos_net_send_all(clients[index], sent_messages[index],
                                                (size_t)length, &sent, 2000U) ==
                  LINKOS_NET_OK && sent == (size_t)length,
              "send on a simultaneous client connection");
    }
    for (index = 0U; index < PEER_COUNT; index++) {
        check(linkos_net_accept(listener, 2000U, &servers[index]) == LINKOS_NET_OK,
              "accept a simultaneous client connection");
    }
    check(linkos_net_cleanup() == LINKOS_NET_BUSY,
          "network shutdown is refused while peer sockets are active");
    {
        uint8_t write_interests[PEER_COUNT];
        uint8_t write_ready[PEER_COUNT];
        memset(write_interests, LINKOS_NET_EVENT_WRITE, sizeof(write_interests));
        check(linkos_net_poll(clients, write_interests, write_ready, PEER_COUNT, 2000U) ==
                  LINKOS_NET_OK,
              "poll multiple peer sockets for write readiness");
        for (index = 0U; index < PEER_COUNT; index++) {
            check((write_ready[index] & LINKOS_NET_EVENT_WRITE) != 0U,
                  "connected peers report writable readiness");
        }
    }

    {
        size_t attempt;
        int all_ready = 0;
        for (attempt = 0U; attempt < 20U && !all_ready; attempt++) {
            linkos_net_error_t poll_result = linkos_net_poll(
                servers, interests, ready, PEER_COUNT, 100U);
            size_t peer;
            if (poll_result != LINKOS_NET_OK && poll_result != LINKOS_NET_TIMEOUT) {
                break;
            }
            for (peer = 0U; peer < PEER_COUNT; peer++) {
                observed[peer] |= ready[peer];
            }
            all_ready = 1;
            for (peer = 0U; peer < PEER_COUNT; peer++) {
                if ((observed[peer] & LINKOS_NET_EVENT_READ) == 0U) {
                    all_ready = 0;
                }
            }
        }
        check(all_ready, "poll multiple peer sockets until all are ready");
    }
    for (index = 0U; index < PEER_COUNT; index++) {
        int length = (int)strlen(sent_messages[index]);
        check((observed[index] & LINKOS_NET_EVENT_READ) != 0U,
              "all active peer sockets become readable");
        check(receive_exact(servers[index], received_messages, (size_t)length) &&
                  memcmp(received_messages, sent_messages[index], (size_t)length) == 0,
              "receive data from each simultaneously active peer");
    }

    for (index = 0U; index < PEER_COUNT; index++) {
        char response[24];
        int length = snprintf(response, sizeof(response), "peer-%u-response",
                              (unsigned int)index);
        check(length > 0 && linkos_net_send_all(servers[index], response,
                                                (size_t)length, &sent, 2000U) ==
                  LINKOS_NET_OK && sent == (size_t)length,
              "send response to each active peer");
    }
    memset(observed, 0, sizeof(observed));
    {
        size_t attempt;
        int all_ready = 0;
        for (attempt = 0U; attempt < 20U && !all_ready; attempt++) {
            linkos_net_error_t poll_result = linkos_net_poll(
                clients, interests, ready, PEER_COUNT, 100U);
            size_t peer;
            if (poll_result != LINKOS_NET_OK && poll_result != LINKOS_NET_TIMEOUT) {
                break;
            }
            for (peer = 0U; peer < PEER_COUNT; peer++) {
                observed[peer] |= ready[peer];
            }
            all_ready = 1;
            for (peer = 0U; peer < PEER_COUNT; peer++) {
                if ((observed[peer] & LINKOS_NET_EVENT_READ) == 0U) {
                    all_ready = 0;
                }
            }
        }
        check(all_ready, "poll client sockets until all responses are ready");
    }
    for (index = 0U; index < PEER_COUNT; index++) {
        char expected[24];
        int length = snprintf(expected, sizeof(expected), "peer-%u-response",
                              (unsigned int)index);
        check((observed[index] & LINKOS_NET_EVENT_READ) != 0U && length > 0 &&
                  receive_exact(clients[index], received_messages, (size_t)length) &&
                  memcmp(received_messages, expected, (size_t)length) == 0,
              "receive simultaneous peer responses");
    }

    /* send_some is a single bounded nonblocking attempt; send_all completes the rest. */
    memset(large_outgoing, 'P', sizeof(large_outgoing));
    send_result = linkos_net_send_some(clients[0], large_outgoing,
                                       sizeof(large_outgoing), &first_send);
    if (send_result == LINKOS_NET_WOULD_BLOCK) {
        uint8_t write_interest = LINKOS_NET_EVENT_WRITE;
        uint8_t write_ready = 0U;
        linkos_net_socket_t *one_client[1] = {clients[0]};
        check(linkos_net_poll(one_client, &write_interest, &write_ready, 1U, 2000U) ==
                  LINKOS_NET_OK &&
                  (write_ready & LINKOS_NET_EVENT_WRITE) != 0U,
              "wait for a nonblocking client send to become writable");
        send_result = linkos_net_send_some(clients[0], large_outgoing,
                                           sizeof(large_outgoing), &first_send);
    }
    check(send_result == LINKOS_NET_OK && first_send > 0U &&
              first_send < sizeof(large_outgoing),
          "single nonblocking send reports an actual partial byte count");
    check(linkos_net_receive(servers[0], large_incoming, 257U, &first_receive, 2000U) ==
              LINKOS_NET_OK && first_receive > 0U && first_receive < first_send,
          "receive reports a partial read when capacity is smaller than queued data");
    check(receive_exact(servers[0], large_incoming + first_receive,
                        first_send - first_receive) &&
              memcmp(large_incoming, large_outgoing, first_send) == 0,
          "receive the remainder of the reported partial send");
    check(linkos_net_send_all(clients[0], large_outgoing + first_send,
                              1024U, &sent, 2000U) == LINKOS_NET_OK && sent == 1024U,
          "send_all reports all bytes sent on success");
    check(receive_exact(servers[0], large_incoming, 1024U) &&
              memcmp(large_incoming, large_outgoing + first_send, 1024U) == 0,
          "send_all payload arrives in order");

    check(linkos_net_close(&clients[0]) == LINKOS_NET_OK,
          "close a peer while other peer sessions remain active");
    check(linkos_net_receive(servers[0], received_messages, sizeof(received_messages),
                             &received_after_close, 2000U) == LINKOS_NET_CLOSED &&
              received_after_close == 0U,
          "detect closure of one peer independently");
    for (index = 0U; index < PEER_COUNT; index++) {
        (void)linkos_net_close(&clients[index]);
        (void)linkos_net_close(&servers[index]);
    }
}

static void test_socket_resource_bound(void)
{
    enum { PEER_COUNT = 30 };
    linkos_net_socket_t *listener = NULL;
    linkos_net_socket_t *clients[PEER_COUNT] = {NULL};
    linkos_net_socket_t *servers[PEER_COUNT] = {NULL};
    linkos_net_socket_t *pending_clients[2] = {NULL, NULL};
    linkos_net_socket_t *overflow_listener = NULL;
    linkos_net_socket_t *extra_client = NULL;
    linkos_net_socket_t *extra_server = NULL;
    uint16_t port = 0U;
    size_t index;

    check(linkos_net_listen("127.0.0.1", 0U, 64, &listener) == LINKOS_NET_OK &&
              linkos_net_local_port(listener, &port) == LINKOS_NET_OK,
          "start listener for socket resource limit test");
    for (index = 0U; index < PEER_COUNT; index++) {
        check(linkos_net_connect("127.0.0.1", port, 2000U, &clients[index]) ==
                  LINKOS_NET_OK &&
                  linkos_net_accept(listener, 2000U, &servers[index]) == LINKOS_NET_OK,
              "open a connection within the socket resource bound");
    }
    check(linkos_net_connect("127.0.0.1", port, 2000U, &pending_clients[0]) ==
                  LINKOS_NET_OK &&
              linkos_net_connect("127.0.0.1", port, 2000U, &pending_clients[1]) ==
                  LINKOS_NET_OK,
          "queue connections while capacity remains");
    check(linkos_net_connect("127.0.0.1", port, 2000U, &extra_client) ==
              LINKOS_NET_RESOURCE_LIMIT && extra_client == NULL,
          "reject connect before creating a socket beyond the resource bound");
    check(linkos_net_listen("127.0.0.1", 0U, 8, &overflow_listener) ==
                  LINKOS_NET_RESOURCE_LIMIT && overflow_listener == NULL,
          "reject listen before creating a socket beyond the resource bound");
    check(linkos_net_accept(listener, 1U, &extra_server) ==
                  LINKOS_NET_RESOURCE_LIMIT && extra_server == NULL,
          "reject accept at capacity without consuming a queued connection");

    (void)linkos_net_close(&clients[0]);
    (void)linkos_net_close(&servers[0]);
    check(linkos_net_accept(listener, 2000U, &extra_server) == LINKOS_NET_OK,
          "accept queued connection after capacity is released");
    (void)linkos_net_close(&extra_server);
    (void)linkos_net_close(&pending_clients[0]);
    check(linkos_net_accept(listener, 2000U, &extra_server) == LINKOS_NET_OK,
          "capacity rejection preserves the next queued connection");
    (void)linkos_net_close(&extra_server);
    (void)linkos_net_close(&pending_clients[1]);
    check(linkos_net_connect("127.0.0.1", port, 2000U, &extra_client) == LINKOS_NET_OK &&
              linkos_net_accept(listener, 2000U, &extra_server) == LINKOS_NET_OK,
          "recover socket capacity after peers close");

    for (index = 1U; index < PEER_COUNT; index++) {
        (void)linkos_net_close(&clients[index]);
        (void)linkos_net_close(&servers[index]);
    }
    (void)linkos_net_close(&extra_client);
    (void)linkos_net_close(&extra_server);
    check(linkos_net_close(&listener) == LINKOS_NET_OK,
          "release all sockets after resource limit test");
}

int main(void)
{
    linkos_net_socket_t *listener = NULL;
    uint16_t port = 0U;

    check(linkos_net_listen("127.0.0.1", 0U, 8, &listener) ==
              LINKOS_NET_NOT_INITIALIZED,
          "operations require network initialization");
    check(linkos_net_initialize() == LINKOS_NET_OK, "network initializes");
    check(linkos_net_initialize() == LINKOS_NET_OK, "network initialization is idempotent");
    check(linkos_net_listen("127.0.0.1", 0U, 8, &listener) == LINKOS_NET_OK,
          "loopback listener starts on an ephemeral port");
    check(listener != NULL, "listener handle is returned");
    check(linkos_net_cleanup() == LINKOS_NET_BUSY,
          "cleanup refuses while sockets remain open");
    check(linkos_net_local_port(listener, &port) == LINKOS_NET_OK && port != 0U,
          "ephemeral listener port is discoverable");
    {
        linkos_net_socket_t *only_socket[1] = {listener};
        uint8_t interest = LINKOS_NET_EVENT_READ;
        uint8_t ready = 0U;
        check(linkos_net_poll(only_socket, &interest, &ready, 1U, 25U) ==
                  LINKOS_NET_TIMEOUT && ready == 0U,
              "multi-socket poll has a bounded timeout");
    }
    check(linkos_net_accept(listener, 25U, NULL) == LINKOS_NET_INVALID_ARGUMENT,
          "accept rejects a missing output handle");
    {
        linkos_net_socket_t *unused = NULL;
        check(linkos_net_listen("127.0.0.1", 0U, 0, &unused) ==
                  LINKOS_NET_INVALID_ARGUMENT,
              "listen rejects a nonpositive backlog");
        check(linkos_net_connect(NULL, 80U, 10U, &unused) ==
                  LINKOS_NET_INVALID_ARGUMENT,
              "connect rejects a missing host");
    }

    if (listener != NULL && port != 0U) {
        linkos_net_socket_t *accepted = NULL;
        check(linkos_net_accept(listener, 25U, &accepted) == LINKOS_NET_TIMEOUT &&
                  accepted == NULL,
              "accept timeout is bounded and returns no handle");
        test_connection_cycle(listener, port, 1U);
        test_connection_cycle(listener, port, 2U);
        test_simultaneous_connections(listener, port);
        test_socket_resource_bound();
    }

    check(linkos_net_close(&listener) == LINKOS_NET_OK, "listener closes cleanly");
    check(linkos_net_close(&listener) == LINKOS_NET_OK, "closing a null handle is safe");
    check(linkos_net_cleanup() == LINKOS_NET_OK, "network cleanup succeeds without sockets");
    check(linkos_net_initialize() == LINKOS_NET_OK, "network can be initialized again");
    check(linkos_net_cleanup() == LINKOS_NET_OK, "repeated initialize/cleanup succeeds");
    check(strcmp(linkos_net_error_string(LINKOS_NET_TIMEOUT), "operation timed out") == 0,
          "portable errors have stable descriptions");

    if (failures != 0) {
        fprintf(stderr, "%d network test(s) failed.\n", failures);
        return 1;
    }
    puts("All network tests passed.");
    return 0;
}
