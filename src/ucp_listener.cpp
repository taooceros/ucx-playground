#include "ucp_listener.hpp"
#include "fmt/core.h"
#include "helper.hpp"
#include "ucx_helper.hpp"
#include <arpa/inet.h>
#include <cstdio>
#include <ucp/api/ucp_def.h>

void listener_callback(ucp_conn_request_h conn_request, void *arg) {
    auto task =
        static_cast<async::auto_reset_event<ucp_conn_request_h> *>(arg);
    task->set_or(conn_request);
}

UcpListener::UcpListener(ucp_worker_h &worker, std::string addr, uint16_t port)
    : worker(worker) {

    struct sockaddr_storage listen_addr;
    ucp_listener_params_t params;

    set_sock_addr(addr.c_str(), port, listen_addr);

    params.field_mask = UCP_LISTENER_PARAM_FIELD_SOCK_ADDR |
                        UCP_LISTENER_PARAM_FIELD_CONN_HANDLER;

    params.sockaddr.addr = (const struct sockaddr *)&listen_addr;
    params.sockaddr.addrlen = sizeof(listen_addr);
    params.conn_handler.arg = &task;
    params.conn_handler.cb = listener_callback;

    auto status = ucp_listener_create(worker, &params, &listener);

    if (status != UCS_OK) {
        throw_with_stacktrace("ucp_listener_create failed: {}",
                              ucs_status_string(status));
    }

    ucp_listener_attr_t attr;

    /* Query the created listener to get the port it is listening on. */
    attr.field_mask = UCP_LISTENER_ATTR_FIELD_SOCKADDR;
    status = ucp_listener_query(listener, &attr);
    if (status != UCS_OK) {
        fprintf(stderr, "failed to query the listener (%s)\n",
                ucs_status_string(status));
        ucp_listener_destroy(listener);
        throw_with_stacktrace("ucp_listener_query failed: {}",
                              ucs_status_string(status));
    }

    fmt::println(
        "server is listening on IP {} port {}\n",
        inet_ntoa(((struct sockaddr_in *)&attr.sockaddr)->sin_addr),
        std::to_string(ntohs(((struct sockaddr_in *)&attr.sockaddr)->sin_port))
            .c_str());
}