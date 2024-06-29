#pragma once

#include "fmt/core.h"
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <ucp/api/ucp.h>

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