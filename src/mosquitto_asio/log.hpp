#pragma once

#include <boost/current_function.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>

#define LOG_PRINT(tag__, color__, msg__)                    \
    do {                                                    \
        std::stringstream stringstream__;                   \
        (stringstream__ msg__);                             \
        std::cout << "\033[" << color__ << "m"              \
                  << tag__ << " - " << stringstream__.str() \
                  << "\033[0m" << '\n';                     \
    } while (0)

#define LOG_ERROR(msg) LOG_PRINT("ERR", "31", msg)
#define LOG_WARNING(msg) LOG_PRINT("WRN", "33", msg)
#define LOG_NOTICE(msg) LOG_PRINT("NOT", "33", msg)
#define LOG_INFO(msg) LOG_PRINT("INF", "32", msg)
#define LOG_DEBUG(msg) LOG_PRINT("DBG", "34", msg)
#define LOG_VERBOSE(msg) LOG_PRINT("VRB", "37", msg)
#define LOG_TRACE() LOG_PRINT("TRC", "35", \
                              << __FILE__ << ':' << __LINE__)
