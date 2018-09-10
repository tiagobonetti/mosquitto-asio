#pragma once

#include "native.hpp"

#include <boost/asio.hpp>

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
