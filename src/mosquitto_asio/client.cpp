#include "client.hpp"

#include "error.hpp"
#include "log.hpp"

#define ENABLE_MOSQUITTO_LOG 1

namespace mosquittoasio {

client::client(io_service& io, char const* client_id, bool clean_session)
    : io_(io),
      timer_(io),
      socket_(io),
      native_handle_(native::create(client_id, clean_session, this)) {
    set_callbacks();
}

client::~client() {
    native::destroy(native_handle_);
}

void client::set_tls(char const* capath) {
    native::set_tls(native_handle_, nullptr, capath, nullptr, nullptr, nullptr);
    native::set_tls_opts(native_handle_, 0, nullptr, nullptr);
}

void client::connect(char const* host, int port, int keep_alive) {
    auto rc = native::connect(native_handle_, host, port, keep_alive);
    if (rc) {
        LOG_ERROR(<< "client::connect; connect failed rc:" << rc
                  << " msg:" << rc.message());
        await_timer_reconnect();
        return;
    }

    await_timer_connect();
}

void client::publish(char const* topic, std::string const& payload,
                     int qos, bool retain) {
    native::publish(native_handle_, nullptr, topic,
                    payload.size(), payload.c_str(), qos, retain);
}

void client::send_subscribe(std::string const& topic, int qos) {
    native::subscribe(native_handle_, nullptr, topic.c_str(), qos);
}

void client::send_unsubscribe(std::string const& topic) {
    native::unsubscribe(native_handle_, nullptr, topic.c_str());
}

void client::await_timer_reconnect() {
    using boost::posix_time::seconds;
    timer_.expires_from_now(seconds(5));
    timer_.async_wait([this](error_code ec) { handle_timer_reconnect(ec); });
}

void client::handle_timer_reconnect(error_code ec) {
    if (ec == boost::system::errc::operation_canceled) {
        return;
    }
    if (ec) {
        throw boost::system::system_error(ec);
    }
    auto rc = native::reconnect(native_handle_);
    if (rc) {
        LOG_ERROR(<< "client::handle_timer_reconnect; reconnect failed ec:"
                  << rc << " msg:" << rc.message());
        await_timer_reconnect();
        return;
    }
    await_timer_connect();
}

void client::await_timer_connect() {
    using boost::posix_time::milliseconds;
    timer_.expires_from_now(milliseconds(100));
    timer_.async_wait([this](error_code ec) { handle_timer_connect(ec); });
}

void client::handle_timer_connect(error_code ec) {
    if (ec == boost::system::errc::operation_canceled) {
        return;
    }
    if (ec) {
        throw boost::system::system_error(ec);
    }

    auto rc = native::loop(native_handle_);
    if (rc == errc::connection_lost) {
        await_timer_reconnect();
        return;
    } else if (rc) {
        throw std::system_error(rc);
    }

    await_timer_connect();
}

void client::await_timer_misc() {
    using boost::posix_time::seconds;
    timer_.expires_from_now(seconds(1));
    timer_.async_wait([this](error_code ec) { handle_timer_misc(ec); });
}

void client::handle_timer_misc(error_code ec) {
    if (ec == boost::system::errc::operation_canceled) {
        return;
    } else if (ec) {
        throw boost::system::system_error(ec);
    }

    auto rc = native::loop_misc(native_handle_);
    if (rc == errc::connection_lost) {
        await_timer_reconnect();
        return;
    } else if (rc) {
        throw std::system_error(rc);
    }

    await_timer_misc();

    // the misc loop may create the need of writting
    await_write();
}

void client::await_read() {
    socket_.async_read_some(
        boost::asio::null_buffers(),
        [this](error_code ec, int) { handle_read(ec); });
}

void client::handle_read(error_code ec) {
    if (ec == boost::system::errc::operation_canceled) {
        return;
    }
    if (ec) {
        throw boost::system::system_error(ec);
    }

    auto rc = native::loop_read(native_handle_);
    if (rc == errc::connection_lost) {
        await_timer_reconnect();
        return;
    } else if (rc) {
        throw std::system_error(rc);
    }

    // we want to be always ready for a read
    await_read();

    // receiving entry may create a need of writing
    await_write();
}

void client::await_write() {
    if (writting_) {
        return;
    }
    auto want_write = native::want_write(native_handle_);
    if (!want_write) {
        return;
    }

    writting_ = true;
    socket_.async_write_some(
        boost::asio::null_buffers(),
        [this](error_code ec, int) { handle_write(ec); });
}

void client::handle_write(error_code ec) {
    if (ec == boost::system::errc::operation_canceled) {
        return;
    }
    writting_ = false;
    if (ec) {
        throw boost::system::system_error(ec);
    }

    auto rc = native::loop_write(native_handle_);
    if (rc == errc::connection_lost) {
        await_timer_reconnect();
        return;
    } else if (rc) {
        throw std::system_error(rc);
    }

    // there may be more to be written, so we schedule a write again
    await_write();
}

void client::assign_socket() {
    auto native_socket = native::get_socket(native_handle_);
    socket_.assign(native_socket);

    // Put the socket into non-blocking mode.
    socket_.non_blocking(true);

    await_read();
    await_write();
    await_timer_misc();
}

void client::release_socket() {
    socket_.release();
}

void client::set_callbacks() {
    // XXX: callbacks are called from inside mosquitto's loop functions
    // so they can happen during a timer or socket handling and generate
    // confusing results; a clean solution is to schedule the callbacks
    // to happen after the handling
    native::set_connect_callback(
        native_handle_,
        [](handle_type*, void* user_data, int rc) {
            auto this_ = static_cast<client*>(user_data);
            this_->io_.post([this_, rc] { this_->on_connect(rc); });
        });

    native::set_disconnect_callback(
        native_handle_,
        [](handle_type*, void* user_data, int rc) {
            auto this_ = static_cast<client*>(user_data);
            this_->io_.post([this_, rc] { this_->on_disconnect(rc); });
        });

    native::set_message_callback(
        native_handle_,
        [](handle_type*, void* user_data, native::message_type const* msg) {
            auto this_ = static_cast<client*>(user_data);
            auto topic = std::string(msg->topic);
            auto payload = std::string(static_cast<char const*>(msg->payload),
                                       msg->payloadlen);
            this_->io_.post([this_, topic, payload] {
                this_->on_message(topic, payload);
            });
        });

#if ENABLE_MOSQUITTO_LOG
    native::set_log_callback(
        native_handle_,
        [](handle_type*, void* user_data, int level, char const* str) {
            auto this_ = static_cast<client*>(user_data);
            auto message = std::string(str);
            this_->io_.post([this_, level, message] {
                this_->on_log(level, message);
            });
        });
#endif
}
void client::on_connect(int rc) {
    if (rc) {
        LOG_ERROR(<< "client::on_connect; connection refused code=" << rc);
        await_timer_reconnect();
        return;
    }

    LOG_VERBOSE(<< "client::on_connect; connected");

    connected_ = true;
    assign_socket();

    publish("mosquitto-asio-test", "connected!", 0, false);
    connected_signal();
}

void client::on_disconnect(int rc) {
    // XXX: mosquitto_loop_misc calls on_disconnect twice,
    // as a workaround we discard the second one here
    if (!connected_) {
        LOG_DEBUG(<< "client::on_disconnect; already disconnected; skipping");
        return;
    }

    connected_ = false;
    release_socket();
    disconnected_signal();

    if (rc) {
        LOG_ERROR(<< "client::on_disconnect; disconnected unexpectedly:"
                     " reconnecting");
        await_timer_reconnect();
        return;
    }
    LOG_INFO(<< "client::on_disconnect; disconnected as expected");
}

void client::on_message(std::string const& topic, std::string const& payload) {
    LOG_INFO(<< "client::on_message; topic:\"" << topic
             << "\" payload:\"" << payload << '\"');

    message_received_signal(topic, payload);
}

void client::on_log([[gnu::unused]] int level,
                    [[gnu::unused]] std::string message) {
#if ENABLE_MOSQUITTO_LOG
    switch (level) {
        case MOSQ_LOG_DEBUG:
            LOG_DEBUG(<< "mosquitto debug: \"" << message << '\"');
            break;
        case MOSQ_LOG_INFO:
            LOG_INFO(<< "mosquitto info: \"" << message << '\"');
            break;
        case MOSQ_LOG_NOTICE:
            LOG_NOTICE(<< "mosquitto notice: \"" << message << '\"');
            break;
        case MOSQ_LOG_WARNING:
            LOG_WARNING(<< "mosquitto warning: \"" << message << '\"');
            break;
        case MOSQ_LOG_ERR:
            LOG_ERROR(<< "mosquitto error: \"" << message << '\"');
            break;
        default:
            LOG_ERROR(<< "unknown log level!");
    }
#endif
}
}  // namespace mosquittoasio
