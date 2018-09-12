#include "subscription.hpp"

#include "wrapper.hpp"

#include "log.hpp"

#include <boost/make_unique.hpp>

#include <iostream>

namespace mosquittoasio {

subscription ::subscription(wrapper& w) : wrapper_(w) {
}

subscription::~subscription() {
    unsubscribe();
}

void subscription::subscribe(std::string topic, int qos, handler_type handler) {
    auto unique_entry = boost::make_unique<entry_type>(
        entry_type{topic, qos, handler});

    entry_ = unique_entry.get();
    wrapper_.register_subscription(std::move(unique_entry));
}

void subscription::unsubscribe() {
    wrapper_.unregister_subscription(entry_);
}
std::string const& subscription::get_topic() const {
    if (entry_ == nullptr) {
        throw std::logic_error("cannot get topic, not subscribed");
    }
    return entry_->topic;
}

}  // namespace mosquittoasio
