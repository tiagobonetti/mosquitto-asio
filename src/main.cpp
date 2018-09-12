#include "mosquitto_asio/library.hpp"
#include "mosquitto_asio/native.hpp"
#include "mosquitto_asio/wrapper.hpp"

#include <cxxabi.h>
#include <boost/stacktrace.hpp>
#include <boost/make_unique.hpp>

#include <iostream>

struct broker_info {
    char const* client_id;
    bool clean_session;
    char const* host;
    int port;
    int keep_alive;
    char const* capath;
};

constexpr broker_info broker_{
    "mosquitto-asio-test",
    true,
    "test.mosquitto.org",
    8883,
    5,
    "/etc/ssl/certs",
};

constexpr broker_info broker{
    "mosquitto-asio-test",
    true,
    "broker.hivemq.com",
    1883,
    5,
    nullptr,
};

mosquittoasio::library g_mosquitto_lib;

int main() {
    boost::asio::io_service io;
    mosquittoasio::wrapper mosquitto(io, broker.client_id, broker.clean_session);
    if (broker.capath) {
        mosquitto.set_tls(broker.capath);
    }

    using subscription = mosquittoasio::subscription;
    auto sub1 = boost::make_unique<subscription>(mosquitto);
    auto sub2 = boost::make_unique<subscription>(mosquitto);
    auto sub3 = boost::make_unique<subscription>(mosquitto);
    auto sub4 = boost::make_unique<subscription>(mosquitto);

    sub4->subscribe("mosquitto-asio/unsub", 0,
                    [&](std::string const& topic,
                        std::string const& payload) {

                        std::cout << "got mosquitto message!\n"
                                  << " subscribed to:\"" << sub4->get_topic()
                                  << "\"\n topic:\"" << topic
                                  << "\"\n payload:\"" << payload
                                  << "\"\n"
                                  << " unsubscribing!\n";

                        sub1.reset();
                        sub2.reset();
                        sub3.reset();
                        sub4.reset();
                    });

    auto make_printer = [](mosquittoasio::subscription const& sub) {
        return [&](std::string const& topic,
                   std::string const& payload) {
            std::cout << "got mosquitto message!\n"
                      << " subscribed to:\"" << sub.get_topic()
                      << "\"\n topic:\"" << topic
                      << "\"\n payload:\"" << payload
                      << "\"\n";
        };
    };
    sub1->subscribe("mosquitto-asio/test", 0, make_printer(*sub1));
    sub2->subscribe("mosquitto-asio/+", 0, make_printer(*sub2));
    sub3->subscribe("mosquitto-asio/#", 0, make_printer(*sub3));

    mosquitto.connect(broker.host, broker.port, broker.keep_alive);

    io.run();

    return EXIT_SUCCESS;
}
