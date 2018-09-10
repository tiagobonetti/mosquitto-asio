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

constexpr broker_info broker{
    "mosquitto-asio-test",
    true,
    "test.mosquitto.org",
    8883,
    5,
    "/etc/ssl/certs",
};

mosquittoasio::library g_mosquitto_lib;

int main() {
    boost::asio::io_service io;
    mosquittoasio::wrapper mosquitto(io, broker.client_id, broker.clean_session);
    mosquitto.set_tls(broker.capath);
    mosquitto.connect(broker.host, broker.port, broker.keep_alive);
    io.run();

    return EXIT_SUCCESS;
}
