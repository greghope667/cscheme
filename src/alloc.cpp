#include "alloc.hpp"
#include "sxi.hpp"

#include <stdlib.h>

void* sxi_malloc(size_t size) {
    auto ptr = ::malloc(size);
    if (not ptr) sxi::error("out of memory");
    return ptr;
}

void* sxi_realloc(void* p, size_t old_size, size_t new_size) {
    (void)old_size;
    auto ptr =  ::realloc(p, new_size);
    if (not ptr) sxi::error("out of memory");
    return ptr;
}

void sxi_free(void* p, size_t size) {
    (void)size;
    ::free(p);
}
