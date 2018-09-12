cmake_minimum_required(VERSION 2.8)
project(mosquitto-asio)

set(CMAKE_BUILD_TYPE Debug)

#mosquitto-asio library

add_library(mosquitto-asio STATIC
    src/mosquitto_asio/error.cpp
    src/mosquitto_asio/native.cpp
    src/mosquitto_asio/subscription.cpp
    src/mosquitto_asio/wrapper.cpp
    )
target_include_directories(mosquitto-asio PRIVATE src)
target_compile_options(mosquitto-asio PRIVATE
    "-std=c++11"
    "-pedantic-errors"
    "-Werror"
    "-Wall"
    "-Wextra"
    )
target_link_libraries(mosquitto-asio
    mosquitto
    boost_system
    )

# test application

add_executable(mosquitto-asio-test
    src/main.cpp
    src/terminate.cpp
    )
target_compile_options(mosquitto-asio-test PRIVATE
    "-std=c++11"
    "-pedantic-errors"
    "-Werror"
    "-Wall"
    "-Wextra"
    )
target_link_libraries(mosquitto-asio-test
    mosquitto-asio
    dl #boost::stacktrace
    pthread
    )
