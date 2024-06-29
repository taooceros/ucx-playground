#include <cstdlib>
#include <string>
#include <thread>
#include <ucp/api/ucp.h>

#include "fmt/core.h"
#include "fmt/format.h"
#include "ucp_listener.hpp"
#include "ucx_helper.hpp"
#include <helper.hpp>
#include <ucp/api/ucp_def.h>
#include <unistd.h>

struct task {
    struct promise_type {
        task get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() {}
    };
};

task start_server(UcpListener &UcpListener) {
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
}

int main(int argc, char **argv) {
    ucp_context_h ucp_context;

    ucp_worker_h ucp_worker;

    init_context(&ucp_context, &ucp_worker);

    UcpListener listener(ucp_worker, getenv_throw("SERVER_IP"),
                         std::stoi(getenv_throw("SERVER_PORT")));

    start_server(listener);

    while (true) {
        ucp_worker_progress(ucp_worker);
    }
}