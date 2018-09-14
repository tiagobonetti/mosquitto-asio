#include "dispatcher.hpp"

#include "log.hpp"
#include "wrapper.hpp"

namespace mosquittoasio {

dispatcher::dispatcher(wrapper& w)
    : wrapper_(w),
      connected_connection(wrapper_.connected_signal.connect(
          [this]() {
              on_connect();
          })),
      message_received_connection(wrapper_.message_received_signal.connect(
          [this](std::string const& topic, std::string const& payload) {
              on_message(topic, payload);
          })) {
}

subscription dispatcher::subscribe(std::string topic, int qos, handler_type h) {
    auto it = entries_.find(topic);
    if (it == entries_.end()) {
        std::tie(it, std::ignore) =
            entries_.emplace(topic, entry{topic, qos, {}});

        // this is a new entry we need to send the subscribe
        if (wrapper_.is_connected()) {
            wrapper_.send_subscribe(topic, qos);
        }
    }

    auto& entry = it->second;
    return subscription{*this, entry.topic, entry.signal.connect(h)};
}

void dispatcher::unsubscribe(std::string const& topic) {
    // XXX: erasing on a separate work to guarantee not beeing nested from a
    // on_message callback, the function can deal with any other callback
    // subscribes or unsubscribes beetween now and the work with no problem
    wrapper_.io_.post([this, topic] {
        auto it = entries_.find(topic);
        if (it != entries_.end()) {
            return;
        }

        auto const& entry = it->second;
        if (entry.signal.empty()) {
            wrapper_.send_unsubscribe(topic);
            entries_.erase(topic);
        }
    });
}

void dispatcher::on_connect() {
    std::for_each(entries_.begin(), entries_.end(),
                  [this](std::pair<std::string, entry const&> pair) {
                      auto const& entry = pair.second;
                      wrapper_.send_subscribe(entry.topic, entry.qos);
                  });
}

void dispatcher::on_message(std::string const& topic,
                            std::string const& payload) {
    LOG_INFO(<< "dispatcher::on_message; topic:\"" << topic
             << "\" payload:\"" << payload << '\"');

    std::for_each(
        entries_.cbegin(), entries_.cend(),
        [this, &topic, &payload](std::pair<std::string, entry const&> pair) {
            auto const& entry = pair.second;
            auto matches = native::topic_matches_subscription(
                entry.topic.c_str(), topic.c_str());

            if (!matches) {
                return;
            }

            entry.signal(topic, payload);
        });
}

}  // namespace mosquittoasio
