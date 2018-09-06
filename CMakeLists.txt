cmake_minimum_required(VERSION 2.8)
project(mosquitto-asio)

set(CMAKE_BUILD_TYPE Debug)

add_executable(mosquitto-asio
    src/main.cpp
    src/native.cpp
    )


add_definitions ("-pedantic-errors -Werror -Wall -Wextra")
 
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
    dl
    )
