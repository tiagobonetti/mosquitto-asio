#include "subscription.hpp"

#include "dispatcher.hpp"

#include "log.hpp"

#include <boost/make_unique.hpp>

#include <iostream>

namespace mosquittoasio {

subscription::~subscription() {
    connection_.disconnect();
    if (dispatcher_ && topic_) {
        dispatcher_->unsubscribe(*topic_);
    }
}

subscription::subscription(subscription&& o)
    : dispatcher_(o.dispatcher_),
      topic_(o.topic_),
      connection_(std::move(connection_)) {
    o.dispatcher_ = nullptr;
    o.topic_ = nullptr;
}

subscription& subscription::operator=(subscription&& o) {
    std::swap(dispatcher_, o.dispatcher_);
    std::swap(topic_, o.topic_);
    std::swap(connection_, o.connection_);
    return *this;
}

subscription::subscription(dispatcher& d, std::string const& t, connection&& c)
    : dispatcher_(&d), topic_(&t), connection_(std::move(c)) {
}

}  // namespace mosquittoasio
