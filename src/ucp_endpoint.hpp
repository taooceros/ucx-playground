#pragma once

#include "src/ucp_listener.hpp"
#include <ucp/api/ucp_def.h>
class UcpEndPoint {
    ucp_ep_h ep;
    ucp_worker_h &worker;
    ucp_task<ucp_ep_h> task;
    std::coroutine_handle<> handle;

    static void ep_close_cb(ucp_ep_h ep, void *arg);

  public:
    UcpEndPoint(ucp_worker_h &worker, ucp_conn_request_h conn_request);

    ucp_task<ucp_ep_h> &connect() { return task; }

    ~UcpEndPoint() { ucp_ep_destroy(ep); }
};