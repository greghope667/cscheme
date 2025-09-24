#pragma once

#include "sxi.hpp"

namespace sxi {

inline int gc_allocations;

template <typename T>
requires(tt::traits<T>::is_boxed)
T* gc_alloc();

void gc_protect(SXI value);

struct Continuation;
void gc_run(Continuation* cont, SXI tos);

}
