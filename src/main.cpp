#include "mosquitto_asio/library.hpp"
#include "mosquitto_asio/native.hpp"
#include "mosquitto_asio/wrapper.hpp"

#include <cxxabi.h>
#include <boost/stacktrace.hpp>

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

    auto print_message = [](mosquittoasio::subscription const& sub,
                            std::string const& topic,
                            std::string const& payload) {
        std::cout << "got mosquitto message!\n"
                  << " subscribed to:\"" << sub.topic
                  << "\"\n topic:\"" << topic
                  << "\"\n payload:\"" << payload
                  << "\"\n";
    };

    auto sub1 = mosquitto.subscribe("mosquitto-asio/test", 0, print_message);
    auto sub2 = mosquitto.subscribe("mosquitto-asio/+", 0, print_message);
    auto sub3 = mosquitto.subscribe("mosquitto-asio/#", 0, print_message);
    auto sub4 = mosquitto.subscribe("mosquitto-asio/unsub", 0,
                                    [&](mosquittoasio::subscription const&,
                                        std::string const&,
                                        std::string const&) {

                                        mosquitto.unsubscribe(sub1);
                                        mosquitto.unsubscribe(sub2);
                                        mosquitto.unsubscribe(sub3);
                                    });

    mosquitto.connect(broker.host, broker.port, broker.keep_alive);

    io.run();

    return EXIT_SUCCESS;
}
