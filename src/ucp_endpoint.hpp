#pragma once

#include "src/async/auto_reset_event.hpp"
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

    /**
     * Send and receive a message using the Stream API.
     * The client sends a message to the server and waits until the send it
     * completed. The server receives a message from the client and waits for
     * its completion.
     */
    template <typename T>
    inline async::auto_reset_event_handle<void *> send_stream(T *data) {
        ucp_request_param_t param{};

        auto event = new async::auto_reset_event_handle<void *>();
        param.op_attr_mask |= UCP_OP_ATTR_FIELD_USER_DATA |
                              UCP_OP_ATTR_FIELD_DATATYPE |
                              UCP_OP_ATTR_FIELD_CALLBACK;
        param.datatype = ucp_dt_make_contig(1);
        param.user_data = event;
        /* Client sends a message to the server using the stream API */
        param.cb.send = [](void *request, ucs_status_t status,
                           void *user_data) {
            auto event = static_cast<async::auto_reset_event_handle<void *> *>(
                user_data);

            fmt::println("send_stream: status: {}", ucs_status_string(status));

            if (status != UCS_OK) {
                fmt::println(stderr, "send failed: {}",
                             ucs_status_string(status));
                return;
            }

            event->set_or(request);

            delete event;

            ucp_request_free(request);
        };
        ucp_stream_send_nbx(ep, static_cast<void *>(data), sizeof(T),
                            &param);

        return *event;
    }

    /**
     * Receive a message using the Stream API.
     * The server receives a message from the client and waits for its
     * completion.
     */
    template <typename T, size_t _Extent>
    inline async::auto_reset_event_handle<std::pair<void *, size_t>>
    recv_stream(std::span<T, _Extent> buffer) {
        ucp_request_param_t param = {};
        param.op_attr_mask |=
            UCP_OP_ATTR_FIELD_FLAGS | UCP_OP_ATTR_FIELD_USER_DATA |
            UCP_OP_ATTR_FIELD_DATATYPE | UCP_OP_ATTR_FIELD_CALLBACK;
        // param.flags = UCP_STREAM_RECV_FLAG_WAITALL;

        auto event =
            new async::auto_reset_event_handle<std::pair<void *, size_t>>();
        param.datatype = ucp_dt_make_contig(1);

        param.user_data = event;
        /* Server receives a message from the client using the stream API */
        param.cb.recv_stream = [](void *request, ucs_status_t status,
                                  size_t length, void *user_data) {
            auto event = static_cast<
                async::auto_reset_event_handle<std::pair<void *, size_t>> *>(
                user_data);

            if (status != UCS_OK) {
                throw std::runtime_error(
                    fmt::format("recv failed: {}", ucs_status_string(status)));
            }

            event->set_or(std::make_pair(request, length));

            delete event;

            ucp_request_free(request);
        };

        size_t length = buffer.size_bytes();

        auto status = ucp_stream_recv_nbx(ep, buffer.data(),
                                          buffer.size_bytes(), &length, &param);

        if (UCS_PTR_IS_ERR(status)) {
            fmt::println("ucp_stream_recv_nbx failed: {}",
                         ucs_status_string(UCS_PTR_STATUS(status)));

            exit(1);
        }

        return *event;
    }

    const ucp_ep_h get() const { return ep; }
};