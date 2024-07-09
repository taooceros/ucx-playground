#pragma once

#include <ucp/api/ucp.h>
#include <ucp/api/ucp_def.h>

class UcpContext {
    ucp_context_h ucp_context;

    UcpContext(const UcpContext &) = delete;
    UcpContext &operator=(const UcpContext &) = delete;

    void cleanup() { ucp_cleanup(ucp_context); }

  public:
    const ucp_context_h get() const { return ucp_context; }
    UcpContext();
    ~UcpContext();
};