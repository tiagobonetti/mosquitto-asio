#pragma once

#include "native.hpp"

namespace mosquittoasio {

class library {
   public:
    library() { native::lib_init(); }
    ~library() { native::lib_cleanup(); }
};

}  // namespace mosquittoasio
