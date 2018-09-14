#pragma once

#include "subscription.hpp"

#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include <unordered_map>

namespace mosquittoasio {

class wrapper;

class dispatcher {
   public:
    using io_service = boost::asio::io_service;

    using handler_type = std::function<void(std::string const& topic,
                                            std::string const& payload)>;

    using sig_type = boost::signals2::signal<
        void(std::string const& topic, std::string const& payload)>;
    using sub_type = boost::signals2::scoped_connection;

    dispatcher(wrapper&);

    dispatcher(dispatcher const&) = delete;
    dispatcher& operator=(dispatcher const&) = delete;

    dispatcher(dispatcher&&) = default;
    dispatcher& operator=(dispatcher&&) = default;

    subscription subscribe(std::string topic, int qos, handler_type h);

   private:
    struct entry {
        std::string topic;
        int qos;
        sig_type signal;
    };

    void unsubscribe(std::string const& topic);

    void on_connect();
    void on_message(std::string const& topic, std::string const& payload);

    wrapper& wrapper_;

    boost::signals2::scoped_connection connected_connection;
    boost::signals2::scoped_connection message_received_connection;

    std::unordered_map<std::string, entry> entries_;

    friend class subscription;
};
}  // namespace mosquittoasio
