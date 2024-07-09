#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <thread>
#include <ucp/api/ucp.h>

#include "async/task.hpp"
#include "fmt/core.h"
#include "fmt/format.h"
#include "src/ucp_context.hpp"
#include "src/ucp_worker.hpp"
#include "ucp_listener.hpp"
#include "ucx_helper.hpp"
#include <helper.hpp>
#include <ucp/api/ucp_def.h>
#include <unistd.h>

std::atomic_bool completed = false;

task start_server(UcpListener &UcpListener, UcpWorker &ucp_worker) {
    auto conn_request = co_await UcpListener.accept();

    ucp_conn_request_attr_t attr;

    attr.field_mask = UCP_CONN_REQUEST_ATTR_FIELD_CLIENT_ADDR;
    auto status = ucp_conn_request_query(conn_request, &attr);

    if (status != UCS_OK) {
        throw_with_stacktrace("ucp_conn_request_query failed: {}",
                              ucs_status_string(status));
    }

    fmt::println("Server received a connection request from client at address "
                 "{}:{}",
                 sockaddr_get_ip_str(&attr.client_address),
                 sockaddr_get_port_str(&attr.client_address));

    // retrieve endpoint

    auto buf = std::array<uint64_t, 1>();

    ucp_ep_h ep;

    server_create_ep(ucp_worker.get(), conn_request, &ep);

    fmt::println("Server created an endpoint to the client");

    auto event = recv_stream(ucp_worker.get(), ep, std::span{buf});

    co_await *event;

    fmt::println("Server received a message from the client: {}", buf[0]);

    completed = true;
}

int main(int argc, char **argv) {
    UcpContext ucp_context;
    UcpWorker listener_worker(ucp_context);

    UcpListener listener(listener_worker, getenv_throw("SERVER_IP"),
                         std::stoi(getenv_throw("SERVER_PORT")));

    start_server(listener, listener_worker);

    while (!completed) {
        listener_worker.progress();
    }
}