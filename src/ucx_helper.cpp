
#include "helper.hpp"
#include "src/ucp_worker.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ucp/api/ucp.h>

static void err_cb(void *arg, ucp_ep_h ep, ucs_status_t status) {
    printf("error handling callback was invoked with status %d (%s)\n", status,
           ucs_status_string(status));
}

/**
 * Create a ucp worker on the given ucp context.
 */
static void init_worker(ucp_context_h ucp_context, ucp_worker_h *ucp_worker) {
    ucp_worker_params_t worker_params;
    ucs_status_t status;

    memset(&worker_params, 0, sizeof(worker_params));

    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;

    status = ucp_worker_create(ucp_context, &worker_params, ucp_worker);
    if (status != UCS_OK) {
        throw_with_stacktrace("failed to ucp_worker_create ({})\n",
                              ucs_status_string(status));
    }
}

/**
 * Initialize the UCP context and worker.
 */
void init_context(ucp_context_h *ucp_context, ucp_worker_h *ucp_worker) {
    /* UCP objects */
    ucp_params_t ucp_params;
    ucs_status_t status;
    int ret = 0;

    memset(&ucp_params, 0, sizeof(ucp_params));

    /* UCP initialization */
    ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES | UCP_PARAM_FIELD_NAME;
    ucp_params.name = "client_server";

    ucp_params.features = UCP_FEATURE_STREAM;

    status = ucp_init(&ucp_params, NULL, ucp_context);
    if (status != UCS_OK) {
        throw_with_stacktrace("failed to ucp_init ({})\n",
                              ucs_status_string(status));
    }
    try {
        init_worker(*ucp_context, ucp_worker);
    } catch (...) {
        ucp_cleanup(*ucp_context);
        throw;
    }
}

/**
 * Set an address for the server to listen on - INADDR_ANY on a well known port.
 */
void set_sock_addr(const char *address_str, uint16_t port,
                   struct sockaddr_storage &saddr) {

    /* The server will listen on INADDR_ANY */
    memset(&saddr, 0, sizeof(saddr));

    struct sockaddr_in &sa_in = reinterpret_cast<sockaddr_in &>(saddr);

    if (address_str != NULL) {
        inet_pton(AF_INET, address_str, &sa_in.sin_addr);
    } else {
        sa_in.sin_addr.s_addr = INADDR_ANY;
    }
    sa_in.sin_family = AF_INET;
    sa_in.sin_port = htons(port);
}