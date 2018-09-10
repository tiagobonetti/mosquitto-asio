#pragma once

#include <mosquitto.h>

#include <system_error>

namespace mosquittoasio {

enum class errc {
    connection_pending = MOSQ_ERR_CONN_PENDING,
    success = MOSQ_ERR_SUCCESS,
    out_of_memory = MOSQ_ERR_NOMEM,
    protocol = MOSQ_ERR_PROTOCOL,
    invalid_parameters = MOSQ_ERR_INVAL,
    no_connection = MOSQ_ERR_NO_CONN,
    connection_refused = MOSQ_ERR_CONN_REFUSED,
    not_found = MOSQ_ERR_NOT_FOUND,
    connection_lost = MOSQ_ERR_CONN_LOST,
    tls = MOSQ_ERR_TLS,
    payload_size = MOSQ_ERR_PAYLOAD_SIZE,
    not_supported = MOSQ_ERR_NOT_SUPPORTED,
    auth = MOSQ_ERR_AUTH,
    acl_denied = MOSQ_ERR_ACL_DENIED,
    unknown = MOSQ_ERR_UNKNOWN,
    errno_ = MOSQ_ERR_ERRNO,
    eai = MOSQ_ERR_EAI,
    proxy = MOSQ_ERR_PROXY
};

std::error_code make_error_code(errc);

}  // namespace mosquittoasio

namespace std {
template <>
struct is_error_code_enum<mosquittoasio::errc> : true_type {};
}  // namespace std
