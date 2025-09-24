#pragma once

#include "sxi.hpp"
#include "tl.hpp"

namespace sxi {

struct Vector : vector<SXI> {};
template <> struct tt::traits<Vector> : tt::tagged<SXI_TAG_vector>, tt::boxed {};

}