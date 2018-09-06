#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "native.hpp"
#include "wrapper.hpp"

#include <boost/stacktrace.hpp>

#include <cxxabi.h>

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

void terminate();

int main() {
    std::set_terminate(terminate);
    native::lib_init();

    {
        boost::asio::io_service io;
        wrapper mosquitto(io, broker.client_id, broker.clean_session);
        if (broker.capath) {
            mosquitto.set_tls(broker.capath);
        }
        mosquitto.connect(broker.host, broker.port, broker.keep_alive);
        io.run();
    }

    native::lib_cleanup();
    return EXIT_SUCCESS;
}

void terminate() {
    auto excetion_ptr = std::current_exception();
    if (excetion_ptr == nullptr) {
        std::cerr << "terminate for unkonwn reason" << '\n';
    } else {
        auto current_exception_type_name = [] {
            int status;
            auto name = abi::__cxa_current_exception_type()->name();
            return abi::__cxa_demangle(name, 0, 0, &status);
        };
        try {
            std::rethrow_exception(excetion_ptr);
        } catch (const std::exception& e) {
            std::cerr << "terminate on std::exception"
                      << " type: \"" << current_exception_type_name()
                      << "\" what: \"" << e.what()
                      << "\"\n";
        } catch (...) {
            std::cerr << "terminate on exception"
                      << " type: \"" << current_exception_type_name()
                      << "\"\n";
        }
    }
    std::cerr << boost::stacktrace::stacktrace();
    std::exit(EXIT_FAILURE);
}
