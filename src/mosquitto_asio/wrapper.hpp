#pragma once

#include "native.hpp"
#include "subscription.hpp"

#include <boost/asio.hpp>
#include <boost/signals2.hpp>

namespace mosquittoasio {

class wrapper {
   public:
    using io_service = boost::asio::io_service;

    using connected_signal_type = boost::signals2::signal<void()>;
    using disconnected_signal_type = boost::signals2::signal<void()>;
    using message_received_signal_type = boost::signals2::signal<
        void(std::string const& topic, std::string const& payload)>;

    wrapper(io_service& io, char const* client_id = nullptr, bool clean_session = true);
    ~wrapper();

    wrapper(wrapper&&) = default;
    wrapper& operator=(wrapper&&) = default;

    void set_tls(char const* capath);
    void connect(char const* host, int port, int keep_alive);

    bool is_connected() const { return connected_; }

    void publish(char const* topic, std::string const& payload, int qos, bool retain = false);

    connected_signal_type connected_signal;
    disconnected_signal_type disconnected_signal;
    message_received_signal_type message_received_signal;

   private:
    using error_code = boost::system::error_code;

    using timer_type = boost::asio::deadline_timer;
    using socket_type = boost::asio::posix::stream_descriptor;

    using handle_type = native::handle_type;

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

    void send_subscribe(std::string const& topic, int qos);
    void send_unsubscribe(std::string const& topic);

    void on_connect(int rc);
    void on_disconnect(int rc);
    void on_message(std::string const& topic, std::string const& payload);
    void on_log(int level, std::string message);

    io_service& io_;
    timer_type timer_;
    socket_type socket_;

    handle_type* native_handle_;

    bool connected_{false};
    bool writting_{false};

    friend class dispatcher;
};
}  // namespace mosquittoasio
