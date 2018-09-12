#pragma once

#include <mosquitto.h>

#include <system_error>

namespace mosquittoasio {
namespace native {

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

handle_type* create(char const* id, bool clean_session, void* user_data);
void destroy(handle_type* handle) noexcept;

void set_tls(handle_type* handle, char const* cafile, char const* capath, char const* certfile, char const* keyfile, int (*pw_callback)(char* buf, int size, int rwflag, void* user_data));
void set_tls_opts(handle_type* handle, int cert_reqs, char const* tls_version, char const* ciphers);
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
std::error_code disconnect(handle_type* handle) noexcept;

void publish(handle_type* handle, int* mid, char const* topic, int payloadlen, void const* payload, int qos, bool retain);
void subscribe(handle_type* handle, int* mid, char const* sub, int qos);
void unsubscribe(handle_type* handle, int* mid, char const* sub);

std::error_code loop(handle_type* handle, int timeout = -1, int max_packets = 1) noexcept;

int get_socket(handle_type*) noexcept;
bool want_write(handle_type* handle) noexcept;

std::error_code loop_read(handle_type* handle, int max_packets = 1) noexcept;
std::error_code loop_write(handle_type* handle, int max_packets = 1) noexcept;
std::error_code loop_misc(handle_type* handle) noexcept;

char const* strerror(int error_code) noexcept;
bool topic_matches_subscription(char const* subscription, char const* topic);

}  // namespace native
}  // namespace mosquittoasio
