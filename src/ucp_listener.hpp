#pragma once

#include "async/ucp_task.hpp"
#include "src/async/auto_reset_event.hpp"
#include "src/ucp_worker.hpp"
#include <coroutine>
#include <cstdint>
#include <cstring>
#include <helper.hpp>
#include <string>
#include <ucp/api/ucp.h>
#include <ucp/api/ucp_def.h>

class UcpListener {
    ucp_listener_h listener;
    UcpWorker &worker;
    async::auto_reset_event<ucp_conn_request_h> task;
    std::coroutine_handle<> handle;

    static void server_conn_handle_cb(ucp_conn_request_h conn_request,
                                      void *arg);

  public:
    UcpListener(UcpWorker &worker, std::string addr, uint16_t port = -1);

    async::auto_reset_event<ucp_conn_request_h> &accept() { return task; }

    ~UcpListener() { ucp_listener_destroy(listener); }
};
