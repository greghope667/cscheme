#pragma once

#include "sxi.hpp"
#include "tl.hpp"

namespace sxi {

[[noreturn]] void error(SXI obj, const char* msg);
[[noreturn]] void invalid_arguments(SXI function, span<SXI> args);

}