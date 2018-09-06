#pragma once
#include "native.hpp"

#include <boost/asio.hpp>

#include <iostream>

class wrapper {
   public:
    using io_service = boost::asio::io_service;

    wrapper(io_service& io, char const* client_id = nullptr, bool clean_session = true);
    ~wrapper();

    wrapper(wrapper&&) = default;
    wrapper& operator=(wrapper&&) = default;

    void set_tls(char const* capath);
    void connect(char const* host, int port, int keep_alive);

    void publish(char const* topic, std::string const& payload, int qos, bool retain = false);

   private:
    using timer_type = boost::asio::deadline_timer;
    using socket_type = boost::asio::posix::stream_descriptor;

    using system_error = boost::system::system_error;
    using error_code = boost::system::error_code;

    using handle_type = native::handle_type;
    using message_type = native::message_type;

    void await_timer_reconnect();
    void handle_timer_reconnect(error_code ec);

    void await_timer_connect();
    void handle_timer_connect(error_code ec);

    void await_timer_misc();
    void handle_timer_misc(error_code ec);

    void assign_socket();
    void release_socket();

    void await_read();
    void handle_read(error_code ec);
    void await_write();
    void handle_write(error_code ec);

    void set_callbacks();

    void on_connect(int rc);
    void on_disconnect(int rc);
    void on_message(message_type const& message);
    void on_log(int level, char const* str);

    io_service& io_;
    timer_type timer_;
    socket_type socket_;

    handle_type* native_handle_;

    bool connected_{false};
    bool writting_{false};
};

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
        std::cout << "wrapper::connect; connect failed rc:" << rc
                  << " msg:" << rc.message() << '\n';
        await_timer_reconnect();
        return;
    }

    await_timer_connect();
}

void wrapper::publish(char const* topic, std::string const& payload, int qos, bool retain) {
    native::publish(native_handle_, nullptr, topic, payload.size(), payload.c_str(), qos, retain);
}

void wrapper::set_callbacks() {
    // callbacks are called from inside mosquitto's loop functions
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
        [](handle_type*, void* user_data, message_type const* message) {
            auto this_ = static_cast<wrapper*>(user_data);
            this_->io_.post([this_, message] { this_->on_message(*message); });
        });

    native::set_log_callback(
        native_handle_,
        [](handle_type*, void* user_data, int level, char const* str) {
            auto this_ = static_cast<wrapper*>(user_data);
            this_->io_.post([this_, level, str] { this_->on_log(level, str); });
        });
}

void wrapper::await_timer_reconnect() {
    std::cout << "wrapper::await_timer_reconnect\n";
    using boost::posix_time::seconds;
    timer_.expires_from_now(seconds(5));
    timer_.async_wait([this](error_code ec) { handle_timer_reconnect(ec); });
}

void wrapper::handle_timer_reconnect(error_code ec) {
    if (ec == boost::system::errc::operation_canceled) {
        std::cout << "wrapper::handle_timer_reconnec; canceled\n";
        return;
    }
    if (ec) {
        throw boost::system::system_error(ec);
    }
    auto rc = native::reconnect(native_handle_);
    if (rc) {
        std::cout << "wrapper::handle_timer_reconnect; reconnect failed ec:"
                  << rc << " msg:" << rc.message() << '\n';
        await_timer_reconnect();
        return;
    }
    await_timer_connect();
}

void wrapper::await_timer_connect() {
    std::cout << "wrapper::await_timer_connect\n";
    using boost::posix_time::milliseconds;
    timer_.expires_from_now(milliseconds(100));
    timer_.async_wait([this](error_code ec) { handle_timer_connect(ec); });
}

void wrapper::handle_timer_connect(error_code ec) {
    if (ec == boost::system::errc::operation_canceled) {
        std::cout << "wrapper::handle_timer_connec; canceled\n";
        return;
    }
    if (ec) {
        throw boost::system::system_error(ec);
    }

    auto rc = native::loop(native_handle_);
    if (rc == native::mosquitto_errc::connection_lost) {
        await_timer_reconnect();
        return;
    } else if (rc) {
        throw std::system_error(rc);
    }

    await_timer_connect();
}

void wrapper::await_timer_misc() {
    std::cout << "wrapper::await_timer_misc\n";
    using boost::posix_time::seconds;
    timer_.expires_from_now(seconds(1));
    timer_.async_wait([this](error_code ec) { handle_timer_misc(ec); });
}

void wrapper::handle_timer_misc(error_code ec) {
    std::cout << "wrapper::handle_timer_misc\n";
    if (ec == boost::system::errc::operation_canceled) {
        std::cout << "wrapper::handle_timer_misc; canceled\n";
        return;
    } else if (ec) {
        throw boost::system::system_error(ec);
    }

    auto rc = native::loop_misc(native_handle_);
    if (rc == native::mosquitto_errc::connection_lost) {
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
    std::cout << "wrapper::await_read async_read_some\n";
    socket_.async_read_some(
        boost::asio::null_buffers(),
        [this](error_code ec, int) { handle_read(ec); });
}

void wrapper::handle_read(error_code ec) {
    if (ec == boost::system::errc::operation_canceled) {
        std::cout << "wrapper::handle_read; canceled\n";
        return;
    }
    if (ec) {
        std::cout << "wrapper::handle_read; error=" << ec << '\n';
        return;
    }

    std::cout << "wrapper::handle_read; loop_read\n";
    auto rc = native::loop_read(native_handle_);
    if (rc == native::mosquitto_errc::connection_lost) {
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
        std::cout << "wrapper::await_write; already writing\n";
        return;
    }
    auto want_write = native::want_write(native_handle_);
    if (!want_write) {
        std::cout << "wrapper::await_write; no need to write\n";
        return;
    }

    std::cout << "wrapper::await_write; async_write_some\n";
    writting_ = true;
    socket_.async_write_some(
        boost::asio::null_buffers(),
        [this](error_code ec, int) { handle_write(ec); });
}

void wrapper::handle_write(error_code ec) {
    if (ec == boost::system::errc::operation_canceled) {
        std::cout << "wrapper::handle_write; canceled\n";
        return;
    }
    writting_ = false;
    if (ec) {
        std::cout << "wrapper::handle_write; error=" << ec << '\n';
        return;
    }

    std::cout << "wrapper::handle_write; loop_write\n";
    auto rc = native::loop_write(native_handle_);
    if (rc == native::mosquitto_errc::connection_lost) {
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
        std::cout << "wrapper::on_connect; connection refused code=" << rc << '\n';
        await_timer_reconnect();
        return;
    }
    std::cout << "wrapper::on_connect; connected\n";

    connected_ = true;
    assign_socket();
    publish("async-test", "hello world", 2, false);
}

void wrapper::on_disconnect(int rc) {
    // XXX: mosquitto_loop_misc calls on_disconnect twice,
    //      as a workaround we discard the second one here
    if (!connected_) {
        std::cout << "wrapper::on_disconnect; already disconnected; skipping\n";
        return;
    }

    connected_ = false;
    release_socket();

    if (rc) {
        std::cout << "wrapper::on_disconnect; disconnected unexpectedly: reconnecting\n";
        await_timer_reconnect();
        return;
    }
    std::cout << "wrapper::on_disconnect; disconnected as expected\n";
}

void wrapper::on_message(message_type const& message) {
    std::string topic(message.topic);
    std::string payload(static_cast<char*>(message.payload), message.payloadlen);
    std::cout << "wrapper::on_message; topic:\"" << topic << "\" payload:\"" << payload << "\"\n";
}

void wrapper::on_log(int level, char const* str) {
    switch (level) {
        case MOSQ_LOG_DEBUG:
            std::cout << "DBG: " << str << '\n';
            break;
        case MOSQ_LOG_INFO:
            std::cout << "INF: " << str << '\n';
            break;
        case MOSQ_LOG_NOTICE:
            std::cout << "NOT: " << str << '\n';
            break;
        case MOSQ_LOG_WARNING:
            std::cout << "WRN: " << str << '\n';
            break;
        case MOSQ_LOG_ERR:
            std::cout << "ERR: " << str << '\n';
            break;
        default:
            std::cout << "unknown log level!\n";
    }
}
