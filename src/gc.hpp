#pragma once

#include "sxi.hpp"
#include "code.hpp"

namespace sxi {

inline int gc_allocations;

template <tt::Boxed T>
T* gc_alloc();

void gc_protect(SXI value);

struct Continuation;
void gc_run(ExecStack& es, SXI tos);

}
