#pragma once

#include <mosquitto.h>

#include <system_error>

namespace native {

enum class mosquitto_errc {
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

std::error_code make_error_code(mosquitto_errc);

using handle_type = struct mosquitto;
using message_type = struct mosquitto_message;

using connect_callback_type = void(handle_type*, void* user_data, int rc);
using disconnect_callback_type = void(handle_type*, void* user_data, int rc);
using publish_callback_type = void(handle_type*, void* user_data, int mid);
using message_callback_type = void(handle_type*, void* user_data, message_type const* message);
using subscribe_callback_type = void(handle_type*, void* user_data, int mid, int qos_count, int const* granted_qos);
using unsubscribe_callback_type = void(handle_type*, void* user_data, int mid);
using log_callback_type = void(handle_type*, void* user_data, int level, char const* str);

void lib_init();
void lib_cleanup();

handle_type* create(char const* id = nullptr, bool clean_session = true, void* user_data = nullptr);
void destroy(handle_type* handle) noexcept;

void set_tls(handle_type* handle, char const* cafile, char const* capath = nullptr, char const* certfile = nullptr, char const* keyfile = nullptr, int (*pw_callback)(char* buf, int size, int rwflag, void* user_data) = nullptr);
void set_tls_opts(handle_type* handle, int cert_reqs, char const* tls_version = nullptr, char const* ciphers = nullptr);
void set_user_data(handle_type* handle, void* user_data) noexcept;

void set_connect_callback(handle_type* handle, connect_callback_type callback) noexcept;
void set_disconnect_callback(handle_type* handle, disconnect_callback_type callback) noexcept;
void set_publish_callback(handle_type* handle, publish_callback_type callback) noexcept;
void set_message_callback(handle_type* handle, message_callback_type callback) noexcept;
void set_subscribe_callback(handle_type* handle, subscribe_callback_type callback) noexcept;
void set_unsubscribe_callback(handle_type* handle, unsubscribe_callback_type callback) noexcept;
void set_log_callback(handle_type* handle, log_callback_type callback) noexcept;

std::error_code connect(handle_type* handle, char const* host, int port, int keepalive) noexcept;
std::error_code reconnect(handle_type* handle) noexcept;
void disconnect(handle_type* handle);

void publish(handle_type* handle, int* mid, char const* topic, int payloadlen = 0, void const* payload = nullptr, int qos = 0, bool retain = false);
void subscribe(handle_type* handle, int* mid, char const* sub, int qos = 0);
void unsubscribe(handle_type* handle, int* mid, char const* sub);

std::error_code loop(handle_type* handle, int timeout = -1, int max_packets = 1) noexcept;

int get_socket(handle_type*) noexcept;
bool want_write(handle_type* handle) noexcept;

std::error_code loop_read(handle_type* handle, int max_packets = 1) noexcept;
std::error_code loop_write(handle_type* handle, int max_packets = 1) noexcept;
std::error_code loop_misc(handle_type* handle) noexcept;

char const* strerror(int error_code) noexcept;

}  // namespace native

namespace std {
template <>
struct is_error_code_enum<native::mosquitto_errc> : true_type {};
}  // namespace std
