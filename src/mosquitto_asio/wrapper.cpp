#include "wrapper.hpp"

#include "error.hpp"

#include <boost/current_function.hpp>

#include <iomanip>
#include <iostream>

#define LOG_PRINT(tag__, color__, msg__)                    \
    do {                                                    \
        std::stringstream stringstream__;                   \
        (stringstream__ msg__);                             \
        std::cout << "\033[" << color__ << "m"              \
                  << tag__ << " - " << stringstream__.str() \
                  << "\033[0m" << '\n';                     \
    } while (0)

#define ERR(msg) LOG_PRINT("ERR", "31", msg)
#define WRN(msg) LOG_PRINT("WRN", "33", msg)
#define NOT(msg) LOG_PRINT("NOT", "33", msg)
#define INF(msg) LOG_PRINT("INF", "32", msg)
#define DBG(msg) LOG_PRINT("DBG", "34", msg)
#define DEV(msg)  // LOG_PRINT("DEV", "1;30;46", msg)
#define TRACE()   // LOG_PRINT("TRACE", "1;30;46", << __LINE__ << " - " << BOOST_CURRENT_FUNCTION)

namespace mosquittoasio {

wrapper::wrapper(io_service& io, char const* client_id, bool clean_session)
    : io_(io),
      timer_(io),
      socket_(io),
      native_handle_(native::create(client_id, clean_session, this)) {
    set_callbacks();
}

wrapper::~wrapper() {
    native::destroy(native_handle_);
}

void wrapper::set_tls(char const* capath) {
    native::set_tls(native_handle_, nullptr, capath, nullptr, nullptr, nullptr);
    native::set_tls_opts(native_handle_, 0, nullptr, nullptr);
}

void wrapper::connect(char const* host, int port, int keep_alive) {
    auto rc = native::connect(native_handle_, host, port, keep_alive);
    if (rc) {
        ERR(<< "wrapper::connect; connect failed rc:" << rc
            << " msg:" << rc.message());
        await_timer_reconnect();
        return;
    }

    await_timer_connect();
}

void wrapper::publish(char const* topic, std::string const& payload, int qos, bool retain) {
    native::publish(native_handle_, nullptr, topic, payload.size(), payload.c_str(), qos, retain);
}

auto wrapper::subscribe(std::string topic, int qos, subscription::handler_type handler) -> subscription_ptr {
    if (!handler) {
        return nullptr;
    }

    INF(<< "subscribe; topic: " << topic << " qos: " << qos);

    auto sub = create_subscription(topic, qos, handler);

    if (connected_) {
        send_subscribe(*sub);
    }
    return sub;
}

void wrapper::unsubscribe(subscription_ptr sub) {
    if (!sub) {
        return;
    }
    if (connected_) {
        send_unsubscribe(*sub);
    }
    sub.reset();
    clear_expired();
}

// internal

auto wrapper::create_subscription(std::string topic, int qos, subscription::handler_type handler) -> subscription_ptr {
    auto sub = std::make_shared<subscription>(subscription{topic, qos, handler});
    subscriptions_.push_back(std::weak_ptr<subscription>(sub));
    return sub;
}

void wrapper::clear_expired() {
    auto remove_it = std::remove_if(
        subscriptions_.begin(), subscriptions_.end(),
        [](subscription_wptr weak) { return weak.expired(); });

    subscriptions_.erase(remove_it);
}

void wrapper::send_subscribe(subscription const& sub) {
    native::subscribe(native_handle_, nullptr, sub.topic.c_str(), sub.qos);
}

void wrapper::send_unsubscribe(subscription const& sub) {
    native::unsubscribe(native_handle_, nullptr, sub.topic.c_str());
}

void wrapper::set_callbacks() {
    // XXX: callbacks are called from inside mosquitto's loop functions
    // so they can happen during a timer or socket handling and generate
    // confusing results; a clean solution is to schedule the callbacks
    // to happen after the handling
    native::set_connect_callback(
        native_handle_,
        [](handle_type*, void* user_data, int rc) {
            auto this_ = static_cast<wrapper*>(user_data);
            this_->io_.post([this_, rc] { this_->on_connect(rc); });
        });

    native::set_disconnect_callback(
        native_handle_,
        [](handle_type*, void* user_data, int rc) {
            auto this_ = static_cast<wrapper*>(user_data);
            this_->io_.post([this_, rc] { this_->on_disconnect(rc); });
        });

    native::set_message_callback(
        native_handle_,
        [](handle_type*, void* user_data, native::message_type const* msg) {
            auto this_ = static_cast<wrapper*>(user_data);
            auto topic = std::string(msg->topic);
            auto payload = std::string(static_cast<char const*>(msg->payload),
                                       msg->payloadlen);
            this_->io_.post([this_, topic, payload] {
                this_->on_message(topic, payload);
            });
        });

    native::set_log_callback(
        native_handle_,
        [](handle_type*, void* user_data, int level, char const* str) {
            auto this_ = static_cast<wrapper*>(user_data);
            auto message = std::string(str);
            this_->io_.post([this_, level, message] {
                this_->on_log(level, message);
            });
        });
}

void wrapper::await_timer_reconnect() {
    DEV(<< "wrapper::await_timer_reconnect");
    using boost::posix_time::seconds;
    timer_.expires_from_now(seconds(5));
    timer_.async_wait([this](error_code ec) { handle_timer_reconnect(ec); });
}

void wrapper::handle_timer_reconnect(error_code ec) {
    if (ec == boost::system::errc::operation_canceled) {
        DEV(<< "wrapper::handle_timer_reconnec; canceled");
        return;
    }
    if (ec) {
        throw boost::system::system_error(ec);
    }
    auto rc = native::reconnect(native_handle_);
    if (rc) {
        ERR(<< "wrapper::handle_timer_reconnect; reconnect failed ec:"
            << rc << " msg:" << rc.message());
        await_timer_reconnect();
        return;
    }
    await_timer_connect();
}

void wrapper::await_timer_connect() {
    DEV(<< "wrapper::await_timer_connect");
    using boost::posix_time::milliseconds;
    timer_.expires_from_now(milliseconds(100));
    timer_.async_wait([this](error_code ec) { handle_timer_connect(ec); });
}

void wrapper::handle_timer_connect(error_code ec) {
    if (ec == boost::system::errc::operation_canceled) {
        DEV(<< "wrapper::handle_timer_connec; canceled");
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

void wrapper::await_timer_misc() {
    DEV(<< "wrapper::await_timer_misc");
    using boost::posix_time::seconds;
    timer_.expires_from_now(seconds(1));
    timer_.async_wait([this](error_code ec) { handle_timer_misc(ec); });
}

void wrapper::handle_timer_misc(error_code ec) {
    DEV(<< "wrapper::handle_timer_misc");
    if (ec == boost::system::errc::operation_canceled) {
        DEV(<< "wrapper::handle_timer_misc; canceled");
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

void wrapper::assign_socket() {
    auto native_socket = native::get_socket(native_handle_);
    socket_.assign(native_socket);

    // Put the socket into non-blocking mode.
    socket_.non_blocking(true);

    await_read();
    await_write();
    await_timer_misc();
}

void wrapper::release_socket() {
    socket_.release();
}

void wrapper::await_read() {
    DEV(<< "wrapper::await_read async_read_some");
    socket_.async_read_some(
        boost::asio::null_buffers(),
        [this](error_code ec, int) { handle_read(ec); });
}

void wrapper::handle_read(error_code ec) {
    if (ec == boost::system::errc::operation_canceled) {
        DEV(<< "wrapper::handle_read; canceled");
        return;
    }
    if (ec) {
        ERR(<< "wrapper::handle_read; error=" << ec);
        return;
    }

    DEV(<< "wrapper::handle_read; loop_read");
    auto rc = native::loop_read(native_handle_);
    if (rc == errc::connection_lost) {
        await_timer_reconnect();
        return;
    } else if (rc) {
        throw std::system_error(rc);
    }

    // we want to be always ready for a read
    await_read();

    // receiving data may create a need of writing
    await_write();
}

void wrapper::await_write() {
    if (writting_) {
        DEV(<< "wrapper::await_write; already writing");
        return;
    }
    auto want_write = native::want_write(native_handle_);
    if (!want_write) {
        DEV(<< "wrapper::await_write; no need to write");
        return;
    }

    DEV(<< "wrapper::await_write; async_write_some");
    writting_ = true;
    socket_.async_write_some(
        boost::asio::null_buffers(),
        [this](error_code ec, int) { handle_write(ec); });
}

void wrapper::handle_write(error_code ec) {
    if (ec == boost::system::errc::operation_canceled) {
        DEV(<< "wrapper::handle_write; canceled");
        return;
    }
    writting_ = false;
    if (ec) {
        DEV(<< "wrapper::handle_write; error=" << ec);
        return;
    }

    DEV(<< "wrapper::handle_write; loop_write");
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

void wrapper::on_connect(int rc) {
    if (rc) {
        ERR(<< "wrapper::on_connect; connection refused code=" << rc);
        await_timer_reconnect();
        return;
    }
    DEV(<< "wrapper::on_connect; connected");

    connected_ = true;
    assign_socket();

    std::for_each(subscriptions_.begin(), subscriptions_.end(),
                  [this](subscription_wptr& weak) {
                      auto shared_sub = weak.lock();
                      if (!shared_sub) {
                          return;
                      }
                      send_subscribe(*shared_sub);
                  });

    publish("mosquitto-asio-test", "connected!", 0, false);
}

void wrapper::on_disconnect(int rc) {
    // XXX: mosquitto_loop_misc calls on_disconnect twice,
    // as a workaround we discard the second one here
    if (!connected_) {
        INF(<< "wrapper::on_disconnect; already disconnected; skipping");
        return;
    }

    connected_ = false;
    release_socket();

    if (rc) {
        ERR(<< "wrapper::on_disconnect; disconnected unexpectedly: reconnecting");
        await_timer_reconnect();
        return;
    }
    INF(<< "wrapper::on_disconnect; disconnected as expected");
}

void wrapper::on_message(std::string const& topic, std::string const& payload) {
    INF(<< "wrapper::on_message; topic:\"" << topic
        << "\" payload:\"" << payload << '\"');

    TRACE();

    auto shared_topic = std::make_shared<std::string>(topic);
    auto shared_payload = std::make_shared<std::string>(payload);

    bool missed = false;

    auto dispatcher = [&](subscription_wptr weak) {
        TRACE();

        auto shared_sub = weak.lock();
        if (!shared_sub) {
            missed = true;
            return;
        }

        TRACE();

        auto matches = native::topic_matches_subscription(
            shared_sub->topic.c_str(), topic.c_str());

        if (matches) {
            TRACE();

            io_.post([this, weak, shared_topic, shared_payload] {
                auto shared_sub = weak.lock();
                if (!shared_sub) {
                    return;
                }

                TRACE();
                shared_sub->handler(*shared_sub, *shared_topic, *shared_payload);
            });
        }
    };
    std::for_each(subscriptions_.cbegin(), subscriptions_.cend(), dispatcher);

    if (missed) {
        clear_expired();
    }
}

void wrapper::on_log(int level, [[gnu::unused]] std::string message) {
    switch (level) {
        case MOSQ_LOG_DEBUG:
            DBG(<< "mosquitto debug:\"" << message << '\"');
            break;
        case MOSQ_LOG_INFO:
            INF(<< "mosquitto info:\"" << message << '\"');
            break;
        case MOSQ_LOG_NOTICE:
            NOT(<< "mosquitto notice:\"" << message << '\"');
            break;
        case MOSQ_LOG_WARNING:
            WRN(<< "mosquitto warning:\"" << message << '\"');
            break;
        case MOSQ_LOG_ERR:
            ERR(<< "mosquitto error:\"" << message << '\"');
            break;
        default:
            ERR(<< "unknown log level!");
    }
}
}  // namespace mosquittoasio
