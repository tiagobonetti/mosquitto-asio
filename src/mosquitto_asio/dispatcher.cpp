#include "dispatcher.hpp"

#include "client.hpp"
#include "log.hpp"

namespace mosquittoasio {

dispatcher::dispatcher(client& c)
    : client_(c),
      connected_connection(client_.connected_signal.connect(
          [this]() {
              on_connect();
          })),
      message_received_connection(client_.message_received_signal.connect(
          [this](std::string const& topic, std::string const& payload) {
              on_message(topic, payload);
          })) {
}

void dispatcher::unsubscribe(std::string const& topic) {
    // XXX: erasing on a separate work to guarantee not beeing nested from a
    // on_message callback, the function can deal with any other callback
    // subscribes or unsubscribes beetween now and the work with no problem
    client_.io().post([this, topic] { erase_entry(topic); });
}

auto dispatcher::emplace_entry(std::string topic, int qos) -> entry& {
    auto it = entries_.find(topic);
    bool updated;
    if (it == entries_.end()) {
        std::tie(it, updated) = entries_.emplace(topic, entry{topic, qos, {}});
    }

    auto& entry = it->second;
    if (entry.qos < qos) {
        entry.qos = qos;
        updated = true;
    }

    // this is a updated entry (or new)
    // if we are connected we need to send the subscribe
    if (updated && client_.is_connected()) {
        client_.send_subscribe(topic, qos);
    }

    return entry;
}

void dispatcher::erase_entry(std::string const& topic) {
    auto it = entries_.find(topic);
    if (it == entries_.end()) {
        return;
    }

    auto const& entry = it->second;
    if (entry.signal.empty()) {
        client_.send_unsubscribe(topic);
        entries_.erase(topic);
    }
}

void dispatcher::on_connect() {
    using element_type = std::pair<std::string, entry const&>;
    std::for_each(entries_.begin(), entries_.end(), [this](element_type e) {
        auto& entry = e.second;
        client_.send_subscribe(entry.topic, entry.qos);
    });
}

void dispatcher::on_message(std::string const& topic,
                            std::string const& payload) {
    LOG_INFO(<< "dispatcher::on_message; topic:\"" << topic
             << "\" payload:\"" << payload << '\"');

    using element_type = std::pair<std::string, entry const&>;
    std::for_each(entries_.cbegin(), entries_.cend(),
                  [this, &topic, &payload](element_type e) {
                      auto& entry = e.second;
                      auto matches = native::topic_matches_subscription(
                          entry.topic.c_str(), topic.c_str());

                      if (!matches) {
                          return;
                      }

                      entry.signal(topic, payload);
                  });
}

}  // namespace mosquittoasio
