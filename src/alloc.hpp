#pragma once

#include <stddef.h>

void* sxi_malloc(size_t size) __attribute__((malloc));
void* sxi_realloc(void* p, size_t old_size, size_t new_size);
void sxi_free(void* p, size_t size);

namespace sxi {

template <typename T>
inline T* __attribute__((malloc))
alloc(int count) {
    return (T*)sxi_malloc(count * sizeof(T));
}

template <typename T>
inline T*
realloc(T* ptr, int old_count, int new_count) {
    return (T*)sxi_realloc(ptr, old_count * sizeof(T), new_count * sizeof(T));
}

template <typename T, typename I>
inline T*
realloc_x2(T* ptr, I& count) {
    auto p = realloc(ptr, count, count*2);
    count *= 2;
    return p;
}

template <typename T>
inline void
free(T* ptr, int count) {
    sxi_free(ptr, count * sizeof(T));
}

}
