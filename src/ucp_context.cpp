#include "ucp_context.hpp"
#include "helper.hpp"

UcpContext::UcpContext() {
    ucp_config_t *config{};
    ucp_params_t params{};

    ucp_config_read(nullptr, nullptr, &config);

    params.field_mask = UCP_PARAM_FIELD_FEATURES | UCP_PARAM_FIELD_NAME;
    params.features = UCP_FEATURE_STREAM;
    params.name = "server";

    auto status = ucp_init(&params, config, &ucp_context);

    if (status != UCS_OK) {
        cleanup();
        throw_with_stacktrace("failed to ucp_init ({})\n",
                              ucs_status_string(status));
    }

    ucp_config_release(config);
}

UcpContext::~UcpContext() { cleanup(); }