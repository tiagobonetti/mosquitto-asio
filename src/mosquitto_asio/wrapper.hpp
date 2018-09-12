#pragma once

#include "native.hpp"

#include <boost/asio.hpp>

#include <memory>

namespace mosquittoasio {

struct subscription {
    using handler_type = std::function<void(subscription const&,
                                            std::string const& topic,
                                            std::string const& payload)>;

    std::string topic;
    int qos;
    handler_type handler;
};

class wrapper {
   public:
    using io_service = boost::asio::io_service;
    using subscription_ptr = std::shared_ptr<subscription>;


    wrapper(io_service& io, char const* client_id = nullptr, bool clean_session = true);
    ~wrapper();

    wrapper(wrapper&&) = default;
    wrapper& operator=(wrapper&&) = default;

    void set_tls(char const* capath);
    void connect(char const* host, int port, int keep_alive);

    void publish(char const* topic, std::string const& payload, int qos, bool retain = false);

    subscription_ptr subscribe(std::string topic, int qos, subscription::handler_type handler);
    void unsubscribe(subscription_ptr);


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

    subscription_ptr create_subscription(std::string topic, int qos, subscription::handler_type handler);
    void clear_expired();

    void send_subscribe(subscription const&);
    void send_unsubscribe(subscription const&);

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

    using subscription_wptr = std::weak_ptr<subscription>;
    std::vector<subscription_wptr> subscriptions_;
};
}  // namespace mosquittoasio
