#include <cxxabi.h>
#include <boost/stacktrace.hpp>
#include <boost/system/system_error.hpp>

#include <iostream>

void terminate_handler() {
    auto excetion_ptr = std::current_exception();
    if (excetion_ptr) {
        auto get_exception_type = [] {
            int status;
            auto name = abi::__cxa_current_exception_type()->name();
            return abi::__cxa_demangle(name, 0, 0, &status);
        };
        try {
            std::rethrow_exception(excetion_ptr);
        } catch (boost::system::system_error const& e) {
            std::cerr << "terminate on boost::system_error\n"
                      << " type:\"" << get_exception_type() << "\"\n"
                      << " code:\"" << e.code() << "\"\n"
                      << " what:\"" << e.what() << "\"\n";
        } catch (std::system_error const& e) {
            std::cerr << "terminate on std::system_error\n"
                      << " type:\"" << get_exception_type() << "\"\n"
                      << " code:\"" << e.code() << "\"\n"
                      << " what:\"" << e.what() << "\"\n";
        } catch (std::exception const& e) {
            std::cerr << "terminate on std::exception\n"
                      << " type:\"" << get_exception_type() << "\"\n"
                      << " what:\"" << e.what() << "\"\n";
        } catch (...) {
            std::cerr << "terminate on exception\n"
                      << " type:\"" << get_exception_type() << "\"\n";
        }
    } else {
        std::cerr << "terminate for unkonwn reason\n";
    }
    std::cerr << boost::stacktrace::stacktrace();
    std::abort();
}

auto g_terminate_handler = std::set_terminate(terminate_handler);
