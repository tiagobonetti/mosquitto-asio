#include "error.hpp"

#include <cerrno>
#include <cstring>

namespace mosquittoasio {
namespace {

struct mosquitto_error_category : std::error_category {
    const char* name() const noexcept override { return "mosquitto"; }
    std::string message(int ev) const override {
        if (ev == static_cast<int>(errc::errno_)) {
            return std::strerror(errno);
        }
        return strerror(ev);
    }
};

const mosquitto_error_category g_mosquitto_error_category{};
}  // namespace

std::error_code make_error_code(errc ec) {
    return {static_cast<int>(ec), g_mosquitto_error_category};
}

}  // namespace mosquittoasio
