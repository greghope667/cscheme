#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#define sxi_realloc(p, o, n) realloc(p, n)

template <typename T>
struct vector {
    T* data;
    unsigned length, capacity;

    void push(T x) {
        if (length == capacity) {
            unsigned new_cap = capacity < 4 ? 4 : capacity*2;
            data = (T*)sxi_realloc(data, capacity*sizeof(T), new_cap*sizeof(T));
            capacity = new_cap;
        }
        data[length++] = x;
    }

    void reserve(unsigned count) {
        if (count > capacity) {
            data = (T*)sxi_realloc(data, capacity*sizeof(T), count*sizeof(T));
            capacity = count;
        }
    }

    auto begin(this auto&& self) { return self.data; }
    auto end(this auto&& self) { return self.data + self.length; }
};

template <typename T>
struct span {
    T* data;
    size_t length;
    auto begin(this auto&& self) { return self.data; }
    auto end(this auto&& self) { return self.data + self.length; }
};

/* Technically dodgy but probably fine
 * Making a tiny std so that <initializer_list> range-for loops work correctly */
namespace std {
    template<class T>
    struct initializer_list {
        const T* data;
        size_t length;
        constexpr initializer_list(const T* t, size_t s) : data(t), length(s) {}
    };
    template <typename T> constexpr const T* begin(initializer_list<T> i) { return i.data; }
    template <typename T> constexpr const T* end(initializer_list<T> i) { return i.data + i.length; }
}
