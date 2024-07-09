#pragma once

#include "src/ucp_worker.hpp"

#include <ucp/api/ucp_def.h>

class UcpEndPoint {
    ucp_ep_h ep;

    static void ep_close_cb(ucp_ep_h ep, void *arg);

    UcpEndPoint(ucp_ep_h ep) : ep(ep) {}

    UcpEndPoint(const UcpEndPoint &) = delete;

    UcpEndPoint &operator=(const UcpEndPoint &) = delete;

  public:
    UcpEndPoint(UcpWorker &worker, ucp_conn_request_h conn_request);

    UcpEndPoint(UcpWorker &worker, const char *ip, int port);

    ~UcpEndPoint();

    const ucp_ep_h get() const { return ep; }
};