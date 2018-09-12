#pragma once

#include <functional>
#include <memory>
#include <ostream>

namespace mosquittoasio {

class wrapper;

class subscription {
   public:
    using handler_type = std::function<void(std::string const& topic,
                                            std::string const& payload)>;

    struct entry_type {
        std::string topic;
        int qos;
        handler_type handler;
    };

    subscription(wrapper&);
    subscription(subscription&&) = default;
    subscription& operator=(subscription&&) = default;
    ~subscription();

    void subscribe(std::string topic, int qos, handler_type handler);
    void unsubscribe();

    std::string const& get_topic() const;

   private:
    entry_type* entry_;
    wrapper& wrapper_;
};

}  // namespace mosquittoasio
