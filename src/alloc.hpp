#pragma once

#include <stdlib.h>

inline void* sxi_malloc(size_t size) { return ::malloc(size); }
inline void* sxi_realloc(void* p, size_t old_size, size_t new_size) {
    (void)old_size;
    return ::realloc(p, new_size);
}
inline void sxi_free(void* p, size_t size) {
    (void)size;
    ::free(p); 
}

namespace sxi {

template <typename T>
inline T*
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
    I old_count = count;
    count *= 2;
    return realloc(ptr, old_count, count);
}

template <typename T>
inline void
free(T* ptr, int count) {
    sxi_free(ptr, count * sizeof(T));
}

}
