#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <ucp/api/ucp.h>

#include "async/task.hpp"
#include "fmt/core.h"
#include "ucp_listener.hpp"
#include "ucx_helper.hpp"
#include <helper.hpp>
#include <ucp/api/ucp_def.h>
#include <unistd.h>

static void send_cb(void *request, ucs_status_t status, void *user_data) {
    fmt::println("Send callback called with status: {}",
                 ucs_status_string(status));
}

std::atomic_bool completed = false;

static task start_client_worker(ucp_worker_h ucp_worker, ucp_ep_h client_ep) {
    auto data = 5;

    auto event = send_stream(ucp_worker, client_ep, &data);

    co_await *event;

    fmt::println("Client sent a message to the server: {}", data);

    completed = true;
}

int main(int argc, char **argv) {
    ucp_context_h ucp_context;

    ucp_worker_h ucp_worker;

    ucp_ep_h client_ep;

    init_context(&ucp_context, &ucp_worker);

    create_end_point(ucp_worker, getenv_throw("SERVER_IP").c_str(),
                     std::stoi(getenv_throw("SERVER_PORT")), client_ep);

    // send_stream(ucp_worker, client_ep, false, 0);

    start_client_worker(ucp_worker, client_ep);

    while (!completed) {
        ucp_worker_progress(ucp_worker);
    }

    ucp_worker_destroy(ucp_worker);
    ucp_cleanup(ucp_context);
}