#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <ratio>
#include <string>
#include <thread>
#include <ucp/api/ucp.h>

#include "async/task.hpp"
#include "fmt/core.h"
#include "src/ucp_context.hpp"
#include "src/ucp_endpoint.hpp"
#include "src/ucp_worker.hpp"
#include "ucp_listener.hpp"
#include "ucx_helper.hpp"
#include <helper.hpp>
#include <ucp/api/ucp_def.h>
#include <unistd.h>

std::atomic_bool completed = false;

static task start_client_worker(UcpWorker &ucp_worker, UcpEndPoint &ep) {
    auto data = 5;

    auto event = ep.send_stream(&data);

    co_await event;

    fmt::println("Client sent a message to the server: {}", data);

    completed = true;
}

int main(int argc, char **argv) {
    UcpContext context{};

    UcpWorker worker(context);

    UcpEndPoint server_ep(worker, getenv_or_throw("SERVER_IP").c_str(),
                          std::stoi(getenv_or_throw("SERVER_PORT")));

    // send_stream(ucp_worker, client_ep, false, 0);

    start_client_worker(worker, server_ep);

    while (!completed) {
        worker.progress();
    }
}