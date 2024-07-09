#pragma once

#include "helper.hpp"
#include "src/ucp_context.hpp"
#include <ucp/api/ucp.h>
#include <ucp/api/ucp_def.h>

class UcpWorker {
    ucp_worker_h ucp_worker;

    void cleanup() { ucp_worker_destroy(ucp_worker); }

  public:
    UcpWorker(UcpContext &ucp_context);

    ~UcpWorker();

    ucp_worker_h get();

    int progress();
};