#pragma once

#include "fmt/core.h"
#include "fmt/format.h"
#include "helper.hpp"
#include "src/async/auto_reset_event.hpp"
#include <arpa/inet.h>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <span>
#include <string>
#include <sys/socket.h>
#include <ucp/api/ucp.h>
#include <ucs/type/status.h>
#include <utility>

/**
 * Create a ucp worker on the given ucp context.
 */
void init_worker(ucp_context_h ucp_context, ucp_worker_h *ucp_worker);

/**
 * Initialize the UCP context and worker.
 */
void init_context(ucp_context_h *ucp_context, ucp_worker_h *ucp_worker);

void set_sock_addr(const char *address_str, uint16_t port,
                   struct sockaddr_storage &saddr);

inline std::string
sockaddr_get_ip_str(const struct sockaddr_storage *sock_addr) {
    struct sockaddr_in addr_in;
    struct sockaddr_in6 addr_in6;

    std::string ip_str(256, '\0');

    switch (sock_addr->ss_family) {
    case AF_INET:
        memcpy(&addr_in, sock_addr, sizeof(struct sockaddr_in));
        inet_ntop(AF_INET, &addr_in.sin_addr, ip_str.data(), ip_str.length());
        return ip_str;
    case AF_INET6:
        memcpy(&addr_in6, sock_addr, sizeof(struct sockaddr_in6));
        inet_ntop(AF_INET6, &addr_in6.sin6_addr, ip_str.data(),
                  ip_str.length());
        return ip_str;
    default:
        return "Invalid address family";
    }
}

inline std::string
sockaddr_get_port_str(const struct sockaddr_storage *sock_addr) {
    struct sockaddr_in addr_in;
    struct sockaddr_in6 addr_in6;

    switch (sock_addr->ss_family) {
    case AF_INET:
        memcpy(&addr_in, sock_addr, sizeof(struct sockaddr_in));
        return fmt::format("{}", ntohs(addr_in.sin_port));
    case AF_INET6:
        memcpy(&addr_in6, sock_addr, sizeof(struct sockaddr_in6));
        return fmt::format("{}", ntohs(addr_in6.sin6_port));
    default:
        return "Invalid address family";
    }
}

/**
 * Initialize the client side. Create an endpoint from the client side to be
 * connected to the remote server (to the given IP).
 */
ucs_status_t create_end_point(ucp_worker_h ucp_worker, const char *address_str,
                              uint16_t port, ucp_ep_h &client_ep);

/**
 * Send and receive a message using the Stream API.
 * The client sends a message to the server and waits until the send it
 * completed. The server receives a message from the client and waits for its
 * completion.
 */
template <typename T>
inline async::auto_reset_event<void *> *send_stream(ucp_worker_h ucp_worker,
                                                    ucp_ep_h ep, T *data) {
    ucp_request_param_t param{};

    auto event = new async::auto_reset_event<void *>();
    param.op_attr_mask |= UCP_OP_ATTR_FIELD_USER_DATA |
                          UCP_OP_ATTR_FIELD_DATATYPE |
                          UCP_OP_ATTR_FIELD_CALLBACK;
    param.datatype = ucp_dt_make_contig(1);
    param.user_data = event;
    /* Client sends a message to the server using the stream API */
    param.cb.send = [](void *request, ucs_status_t status, void *user_data) {
        auto &event =
            *static_cast<async::auto_reset_event<void *> *>(user_data);

        fmt::println("send_stream: status: {}", ucs_status_string(status));

        if (status != UCS_OK) {
            fmt::println(stderr, "send failed: {}", ucs_status_string(status));
            return;
        }

        event.set_or(request);
    };
    ucp_stream_send_nbx(ep, static_cast<void *>(data), sizeof(T), &param);

    return event;
}

/**
 * Receive a message using the Stream API.
 * The server receives a message from the client and waits for its completion.
 */

template <typename T, size_t _Extent>
inline async::auto_reset_event<std::pair<void *, size_t>> *
recv_stream(ucp_worker_h ucp_worker, ucp_ep_h ep,
            std::span<T, _Extent> buffer) {
    ucp_request_param_t param = {};
    param.op_attr_mask |=
        UCP_OP_ATTR_FIELD_FLAGS | UCP_OP_ATTR_FIELD_USER_DATA |
        UCP_OP_ATTR_FIELD_DATATYPE | UCP_OP_ATTR_FIELD_CALLBACK;
    // param.flags = UCP_STREAM_RECV_FLAG_WAITALL;

    assert(param.op_attr_mask ==
           (UCP_OP_ATTR_FIELD_FLAGS | UCP_OP_ATTR_FIELD_USER_DATA |
            UCP_OP_ATTR_FIELD_DATATYPE | UCP_OP_ATTR_FIELD_CALLBACK));

    auto event = new async::auto_reset_event<std::pair<void *, size_t>>();
    param.datatype = ucp_dt_make_contig(1);

    param.user_data = event;
    /* Server receives a message from the client using the stream API */
    param.cb.recv_stream = [](void *request, ucs_status_t status, size_t length,
                              void *user_data) {
        auto &event =
            *static_cast<async::auto_reset_event<std::pair<void *, size_t>> *>(
                user_data);

        if (status != UCS_OK) {
            throw std::runtime_error(
                fmt::format("recv failed: {}", ucs_status_string(status)));
        }

        event.set_or(std::make_pair(request, length));

        return;
    };

    size_t length = buffer.size_bytes();

    auto status = ucp_stream_recv_nbx(ep, buffer.data(), buffer.size_bytes(),
                                      &length, &param);

    if (UCS_PTR_IS_ERR(status)) {
        fmt::println("ucp_stream_recv_nbx failed: {}",
                     ucs_status_string(UCS_PTR_STATUS(status)));

        exit(1);
    }

    return event;
}

ucs_status_t server_create_ep(ucp_worker_h data_worker,
                              ucp_conn_request_h conn_request,
                              ucp_ep_h *server_ep);