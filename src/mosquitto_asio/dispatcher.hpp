#pragma once

#include "subscription.hpp"

#include <boost/signals2.hpp>

#include <unordered_map>

namespace mosquittoasio {

class client;

class dispatcher {
   public:
    dispatcher(client&);

    dispatcher(dispatcher const&) = delete;
    dispatcher& operator=(dispatcher const&) = delete;

    dispatcher(dispatcher&&) = default;
    dispatcher& operator=(dispatcher&&) = default;

    template <typename Handler>
    subscription subscribe(std::string topic, int qos, Handler&& h);

    void unsubscribe(std::string const& topic);

   private:
    using callback_type = void(std::string const& topic,
                               std::string const& payload);
    using signal_type = boost::signals2::signal<callback_type>;

    struct entry {
        std::string topic;
        int qos;
        signal_type signal;
    };

    entry& emplace_entry(std::string topic, int qos);
    void erase_entry(std::string const& topic);

    void on_connect();
    void on_message(std::string const& topic, std::string const& payload);

    client& client_;

    boost::signals2::scoped_connection connected_connection;
    boost::signals2::scoped_connection message_received_connection;

    std::unordered_map<std::string, entry> entries_;
};

template <typename Handler>
subscription dispatcher::subscribe(std::string topic, int qos, Handler&& h) {
    auto& entry = emplace_entry(topic, qos);
    return subscription{*this, entry.topic,
                        entry.signal.connect(std::forward<Handler>(h))};
}

}  // namespace mosquittoasio
