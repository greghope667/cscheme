#pragma once

#include "sxi.hpp"

namespace sxi {

template <typename T>
requires(tt::traits<T>::is_boxed)
T* gc_alloc() {
    // Temporary testing impl
    return new T();
}

}
