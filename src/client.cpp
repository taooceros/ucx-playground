#include <cstdint>
#include <cstdlib>
#include <string>
#include <ucp/api/ucp.h>

#include "ucp_listener.hpp"
#include "ucx_helper.hpp"
#include <helper.hpp>
#include <ucp/api/ucp_def.h>
#include <unistd.h>

static void err_cb(void *arg, ucp_ep_h ep, ucs_status_t status) {
    printf("error handling callback was invoked with status %d (%s)\n", status,
           ucs_status_string(status));
}

typedef struct test_req {
    int complete;
} test_req_t;

static int fill_request_param(ucp_dt_iov_t *iov, int is_client, void **msg,
                              size_t *msg_length, test_req_t *ctx,
                              ucp_request_param_t *param) {

    *msg = iov[0].buffer;
    *msg_length = iov[0].length;

    ctx->complete = 0;
    param->op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK |
                          UCP_OP_ATTR_FIELD_DATATYPE |
                          UCP_OP_ATTR_FIELD_USER_DATA;
    param->datatype = ucp_dt_make_contig(1);
    param->user_data = ctx;

    return 0;
}
static void common_cb(void *user_data, const char *type_str) {
    test_req_t *ctx;

    if (user_data == NULL) {
        fprintf(stderr, "user_data passed to %s mustn't be NULL\n", type_str);
        return;
    }

    ctx = static_cast<test_req_t *>(user_data);
    ctx->complete = 1;
}

static void send_cb(void *request, ucs_status_t status, void *user_data) {
    common_cb(user_data, "send_cb");
}

/**
 * Send and receive a message using the Stream API.
 * The client sends a message to the server and waits until the send it
 * completed. The server receives a message from the client and waits for its
 * completion.
 */
static int send_recv_stream(ucp_worker_h ucp_worker, ucp_ep_h ep, int is_server,
                            int current_iter) {
    ucp_dt_iov_t *iov =
        static_cast<ucp_dt_iov_t *>(alloca(1 * sizeof(ucp_dt_iov_t)));
    ucp_request_param_t param;
    test_req_t *request;
    size_t msg_length;
    void *msg;
    test_req_t ctx;

    memset(iov, 0, sizeof(*iov));

    if (fill_request_param(iov, !is_server, &msg, &msg_length, &ctx, &param) !=
        0) {
        return -1;
    }

    if (!is_server) {
        /* Client sends a message to the server using the stream API */
        param.cb.send = send_cb;
        request = reinterpret_cast<test_req_t *>(
            ucp_stream_send_nbx(ep, msg, msg_length, &param));
    } else {
        /* Server receives a message from the client using the stream API */
        param.op_attr_mask |= UCP_OP_ATTR_FIELD_FLAGS;
        param.flags = UCP_STREAM_RECV_FLAG_WAITALL;
        param.cb.recv_stream = NULL;
        request = reinterpret_cast<test_req_t *>(
            ucp_stream_recv_nbx(ep, msg, msg_length, &msg_length, &param));
    }

    return 0;
}

/**
 * Initialize the client side. Create an endpoint from the client side to be
 * connected to the remote server (to the given IP).
 */
static ucs_status_t start_client(ucp_worker_h ucp_worker,
                                 const char *address_str, uint16_t port,
                                 ucp_ep_h &client_ep) {
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

int main(int argc, char **argv) {
    ucp_context_h ucp_context;

    ucp_worker_h ucp_worker;

    ucp_ep_h client_ep;

    init_context(&ucp_context, &ucp_worker);

    start_client(ucp_worker, getenv_throw("SERVER_IP").c_str(),
                 std::stoi(getenv_throw("SERVER_PORT")), client_ep);

    send_recv_stream(ucp_worker, client_ep, false, 0);

    while (true) {
        ucp_worker_progress(ucp_worker);
    }

    ucp_worker_destroy(ucp_worker);
    ucp_cleanup(ucp_context);
}