#include "ucp_worker.hpp"

UcpWorker::UcpWorker(UcpContext &ucp_context) {
    ucp_worker_params_t worker_params{};
    ucs_status_t status{};

    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;

    status = ucp_worker_create(ucp_context.get(), &worker_params, &ucp_worker);
    if (status != UCS_OK) {
        cleanup();
        throw_with_stacktrace("failed to ucp_worker_create ({})\n",
                              ucs_status_string(status));
    }
}

UcpWorker::~UcpWorker() { cleanup(); }

ucp_worker_h UcpWorker::get() { return ucp_worker; }

int UcpWorker::progress() { return ucp_worker_progress(ucp_worker); }
