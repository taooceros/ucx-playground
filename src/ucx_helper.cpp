
#include "helper.hpp"
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

ucs_status_t create_end_point(ucp_worker_h ucp_worker, const char *address_str,
                              uint16_t port, ucp_ep_h &client_ep) {
    ucp_ep_params_t ep_params;
    struct sockaddr_storage connect_addr;
    ucs_status_t status;

    set_sock_addr(address_str, port, connect_addr);

    /*
     * Endpoint field mask bits:
     * UCP_EP_PARAM_FIELD_FLAGS             - Use the value of the 'flags'
     * field. UCP_EP_PARAM_FIELD_SOCK_ADDR         - Use a remote sockaddr to
     * connect to the remote peer. UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE - Error
     * handling mode - this flag is temporarily required since the endpoint will
     * be closed with UCP_EP_CLOSE_MODE_FORCE which requires this mode. Once
     * UCP_EP_CLOSE_MODE_FORCE is removed, the error handling mode will be
     * removed.
     */
    ep_params.field_mask =
        UCP_EP_PARAM_FIELD_FLAGS | UCP_EP_PARAM_FIELD_SOCK_ADDR |
        UCP_EP_PARAM_FIELD_ERR_HANDLER | UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE;
    ep_params.err_mode = UCP_ERR_HANDLING_MODE_PEER;
    ep_params.err_handler.cb = err_cb;
    ep_params.err_handler.arg = NULL;
    ep_params.flags = UCP_EP_PARAMS_FLAGS_CLIENT_SERVER;
    ep_params.sockaddr.addr = (struct sockaddr *)&connect_addr;
    ep_params.sockaddr.addrlen = sizeof(connect_addr);

    status = ucp_ep_create(ucp_worker, &ep_params, &client_ep);
    if (status != UCS_OK) {
        throw_with_stacktrace("failed to connect to {} ({})\n", address_str,
                              ucs_status_string(status));
    }

    return status;
}

ucs_status_t server_create_ep(ucp_worker_h data_worker,
                              ucp_conn_request_h conn_request,
                              ucp_ep_h *server_ep) {
    ucp_ep_params_t ep_params;
    ucs_status_t status;

    /* Server creates an ep to the client on the data worker.
     * This is not the worker the listener was created on.
     * The client side should have initiated the connection, leading
     * to this ep's creation */
    ep_params.field_mask =
        UCP_EP_PARAM_FIELD_ERR_HANDLER | UCP_EP_PARAM_FIELD_CONN_REQUEST;
    ep_params.conn_request = conn_request;
    ep_params.err_handler.cb = err_cb;
    ep_params.err_handler.arg = NULL;

    status = ucp_ep_create(data_worker, &ep_params, server_ep);
    if (status != UCS_OK) {
        fprintf(stderr, "failed to create an endpoint on the server: (%s)\n",
                ucs_status_string(status));
    }

    return status;
}
