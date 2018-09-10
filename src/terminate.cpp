#include <cxxabi.h>
#include <boost/stacktrace.hpp>

#include <iostream>

class terminate_handler {
   public:
    terminate_handler() {
        std::set_terminate(handle_terminate);
    }

   private:
    static void handle_terminate() {
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
};

terminate_handler g_terminate_handler;
