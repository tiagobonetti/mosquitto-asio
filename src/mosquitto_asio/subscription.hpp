#pragma once

#include <boost/signals2.hpp>

namespace mosquittoasio {

class dispatcher;

class subscription {
   public:
    subscription() = default;

    subscription(subscription const&) = delete;
    subscription& operator=(subscription const&) = delete;

    subscription(subscription&&);
    subscription& operator=(subscription&&);

    ~subscription();

   private:
    using connection = boost::signals2::connection;

    subscription(dispatcher&, std::string const&, connection&&);

    dispatcher* dispatcher_{nullptr};
    std::string const* topic_{nullptr};
    boost::signals2::connection connection_;

    friend class dispatcher;
};

}  // namespace mosquittoasio
