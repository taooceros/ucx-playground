#include "src/ucp_endpoint.hpp"

#include "src/ucx_helper.hpp"

UcpEndPoint::UcpEndPoint(UcpWorker &worker, const char *ip, int port) {
    ucp_ep_params_t ep_params;
    struct sockaddr_storage connect_addr;
    ucs_status_t status;

    set_sock_addr(ip, port, connect_addr);

    /*
     * Endpoint field mask bits:
     * UCP_EP_PARAM_FIELD_FLAGS             - Use the value of the 'flags'
     * field. UCP_EP_PARAM_FIELD_SOCK_ADDR         - Use a remote sockaddr
     * to connect to the remote peer. UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE -
     * Error handling mode - this flag is temporarily required since the
     * endpoint will be closed with UCP_EP_CLOSE_MODE_FORCE which requires
     * this mode. Once UCP_EP_CLOSE_MODE_FORCE is removed, the error
     * handling mode will be removed.
     */
    ep_params.field_mask =
        UCP_EP_PARAM_FIELD_FLAGS | UCP_EP_PARAM_FIELD_SOCK_ADDR |
        UCP_EP_PARAM_FIELD_ERR_HANDLER | UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE;
    ep_params.err_mode = UCP_ERR_HANDLING_MODE_PEER;
    ep_params.err_handler.cb = [](void *arg, ucp_ep_h ep, ucs_status_t status) {
        if (status != UCS_OK) {
            throw_with_stacktrace("error handling callback: {}",
                                  ucs_status_string(status));
        }
    };
    ep_params.err_handler.arg = NULL;
    ep_params.flags = UCP_EP_PARAMS_FLAGS_CLIENT_SERVER;
    ep_params.sockaddr.addr = (struct sockaddr *)&connect_addr;
    ep_params.sockaddr.addrlen = sizeof(connect_addr);

    status = ucp_ep_create(worker.get(), &ep_params, &ep);
    if (status != UCS_OK) {
        throw_with_stacktrace("failed to connect to {} ({})\n", ip,
                              ucs_status_string(status));
    }
};

UcpEndPoint::UcpEndPoint(UcpWorker &worker, ucp_conn_request_h conn_request) {
    ucp_ep_params_t ep_params;
    ucs_status_t status;

    ep_params.field_mask = UCP_EP_PARAM_FIELD_CONN_REQUEST;
    ep_params.conn_request = conn_request;

    status = ucp_ep_create(worker.get(), &ep_params, &ep);
    if (status != UCS_OK) {
        throw_with_stacktrace("failed to create an endpoint from a connection "
                              "request: {}\n",
                              ucs_status_string(status));
    }
};
UcpEndPoint::~UcpEndPoint() { ucp_ep_destroy(ep); }
