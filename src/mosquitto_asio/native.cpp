#include "native.hpp"
#include "error.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>

namespace mosquittoasio {
namespace native {
namespace detail {

std::error_code make_error_code(int ev) {
    return make_error_code(static_cast<errc>(ev));
}

void throw_on_error(int ev) {
    if (ev) {
        throw std::system_error{make_error_code(ev)};
    }
}
}  // namespace detail

void lib_init() {
    auto rc = mosquitto_lib_init();
    detail::throw_on_error(rc);
}

void lib_cleanup() {
    auto rc = mosquitto_lib_cleanup();
    detail::throw_on_error(rc);
}

handle_type* create(char const* id, bool clean_session, void* user_data) {
    auto handle = mosquitto_new(id, clean_session, user_data);
    if (handle == nullptr) {
        detail::throw_on_error(errno);
    }
    return handle;
}

void destroy(handle_type* handle) noexcept {
    mosquitto_destroy(handle);
}

void set_tls(handle_type* handle, char const* cafile, char const* capath, char const* certfile, char const* keyfile, int (*pw_callback)(char* buf, int size, int rwflag, void* user_data)) {
    auto rc = mosquitto_tls_set(handle, cafile, capath, certfile, keyfile, pw_callback);
    detail::throw_on_error(rc);
}

void set_tls_opts(handle_type* handle, int cert_reqs, char const* tls_version, char const* ciphers) {
    auto rc = mosquitto_tls_opts_set(handle, cert_reqs, tls_version, ciphers);
    detail::throw_on_error(rc);
}

void set_user_data(handle_type* handle, void* user_data) noexcept {
    mosquitto_user_data_set(handle, user_data);
}

void set_connect_callback(handle_type* handle, connect_callback_type callback) noexcept {
    mosquitto_connect_callback_set(handle, callback);
}

void set_disconnect_callback(handle_type* handle, disconnect_callback_type callback) noexcept {
    mosquitto_disconnect_callback_set(handle, callback);
}

void set_publish_callback(handle_type* handle, publish_callback_type callback) noexcept {
    mosquitto_publish_callback_set(handle, callback);
}

void set_message_callback(handle_type* handle, message_callback_type callback) noexcept {
    mosquitto_message_callback_set(handle, callback);
}

void set_subscribe_callback(handle_type* handle, subscribe_callback_type callback) noexcept {
    mosquitto_subscribe_callback_set(handle, callback);
}

void set_unsubscribe_callback(handle_type* handle, unsubscribe_callback_type callback) noexcept {
    mosquitto_unsubscribe_callback_set(handle, callback);
}

void set_log_callback(handle_type* handle, log_callback_type callback) noexcept {
    mosquitto_log_callback_set(handle, callback);
}

std::error_code connect(handle_type* handle, char const* host, int port, int keepalive) noexcept {
    auto ev = mosquitto_connect(handle, host, port, keepalive);
    return detail::make_error_code(ev);
}

std::error_code reconnect(handle_type* handle) noexcept {
    auto ev = mosquitto_reconnect(handle);
    return detail::make_error_code(ev);
}

std::error_code disconnect(handle_type* handle) noexcept {
    auto ev = mosquitto_disconnect(handle);
    return detail::make_error_code(ev);
}

void publish(handle_type* handle, int* mid, char const* topic, int payloadlen, void const* payload, int qos, bool retain) {
    auto rc = mosquitto_publish(handle, mid, topic, payloadlen, payload, qos, retain);
    detail::throw_on_error(rc);
}

void subscribe(handle_type* handle, int* mid, char const* sub, int qos) {
    auto rc = mosquitto_subscribe(handle, mid, sub, qos);
    detail::throw_on_error(rc);
}

void unsubscribe(handle_type* handle, int* mid, char const* sub) {
    auto rc = mosquitto_unsubscribe(handle, mid, sub);
    detail::throw_on_error(rc);
}

std::error_code loop(handle_type* handle, int timeout, int max_packets) noexcept {
    auto ev = mosquitto_loop(handle, timeout, max_packets);
    return detail::make_error_code(ev);
}

int get_socket(handle_type* handle) noexcept {
    return mosquitto_socket(handle);
}

bool want_write(handle_type* handle) noexcept {
    return mosquitto_want_write(handle);
}

std::error_code loop_read(handle_type* handle, int max_packets) noexcept {
    auto ev = mosquitto_loop_read(handle, max_packets);
    return detail::make_error_code(ev);
}

std::error_code loop_write(handle_type* handle, int max_packets) noexcept {
    auto ev = mosquitto_loop_write(handle, max_packets);
    return detail::make_error_code(ev);
}

std::error_code loop_misc(handle_type* handle) noexcept {
    auto ev = mosquitto_loop_misc(handle);
    return detail::make_error_code(ev);
}

char const* strerror(int error_code) noexcept {
    return mosquitto_strerror(error_code);
}

bool topic_matches_subscription(char const* subscription, char const* topic) {
    bool matches;
    auto ev = mosquitto_topic_matches_sub(subscription, topic, &matches);
    detail::throw_on_error(ev);
    return matches;
}

}  // namespace native
}  // namespace mosquittoasio
