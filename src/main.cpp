#include "mosquitto_asio/library.hpp"

#include "mosquitto_asio/dispatcher.hpp"
#include "mosquitto_asio/wrapper.hpp"

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

    mosquittoasio::wrapper mosquitto(
        io, broker.client_id, broker.clean_session);

    if (broker.capath) {
        mosquitto.set_tls(broker.capath);
    }

    mosquittoasio::dispatcher disp(mosquitto);

    using subscription = mosquittoasio::subscription;
    auto sub1 = boost::make_unique<subscription>();
    auto sub2 = boost::make_unique<subscription>();
    auto sub3 = boost::make_unique<subscription>();
    auto sub4 = boost::make_unique<subscription>();
    auto sub5 = boost::make_unique<subscription>();

    *sub4 = disp.subscribe(
        "mosquitto-asio/unsub", 0,
        [&](std::string const& topic, std::string const& payload) {
            std::cout << "got mosquitto message!\n"
                      << " subscribed to:\"mosquitto-asio/unsub"
                      << "\"\n topic:\"" << topic
                      << "\"\n payload:\"" << payload
                      << "\"\n"
                      << " unsubscribing!\n";

            if (!sub1) {
                sub4.reset();
                sub5.reset();
            }
            sub1.reset();
            sub2.reset();
            sub3.reset();
        });

    auto make_sub = [&disp](std::string sub_topic) {
        return disp.subscribe(
            sub_topic, 0,
            [sub_topic](std::string const& topic, std::string const& payload) {
                std::cout << "got mosquitto message!\n"
                          << " subscribed to:\"" << sub_topic
                          << "\"\n topic:\"" << topic
                          << "\"\n payload:\"" << payload
                          << "\"\n";
            });
    };

    *sub1 = make_sub("mosquitto-asio/test");
    *sub2 = make_sub("mosquitto-asio/+");
    *sub3 = make_sub("mosquitto-asio/#");

    *sub5 = disp.subscribe(
        "mosquitto-asio/test", 2,
        [&](std::string const& topic, std::string const& payload) {
            std::cout << "got mosquitto message! (2)\n"
                      << " subscribed to:\"mosquitto-asio/test"
                      << "\"\n topic:\"" << topic
                      << "\"\n payload:\"" << payload
                      << "\"\n";
        });

    mosquitto.connect(broker.host, broker.port, broker.keep_alive);

    io.run();

    return EXIT_SUCCESS;
}
