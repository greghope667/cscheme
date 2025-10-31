#pragma once
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "alloc.hpp"

namespace sxi {

[[noreturn]] void error_f(const char*, ...);

static inline void bounds_check(ptrdiff_t index, ptrdiff_t length) {
    if (index < 0 || index >= length)
        sxi::error_f("index out of range (%li > %li)", index, length);
}

template <typename T>
struct span {
    T* data;
    ptrdiff_t length;

    auto begin(this auto&& self) { return self.data; }
    auto end(this auto&& self) { return self.data + self.length; }

    T& operator[](ptrdiff_t i) { bounds_check(i, length); return data[i]; }
};

template <typename T>
struct vector {
    T* data;
    int length, capacity;

    void push(T x) {
        if (length == capacity) {
            int new_cap = capacity < 4 ? 4 : capacity*2;
            data = sxi::realloc(data, capacity, new_cap);
            capacity = new_cap;
        }
        data[length++] = x;
    }

    void reserve(int count) {
        if (count > capacity) {
            data = sxi::realloc(data, capacity, count);
            capacity = count;
        }
    }

    void dealloc() {
        sxi::free(data, capacity);
        *this = {};
    }

    void shrink_to_fit() {
        data = sxi::realloc(data, capacity, length);
        capacity = length;
    }

    auto begin(this auto&& self) { return self.data; }
    auto end(this auto&& self) { return self.data + self.length; }

    auto& back(this auto&& self) { return self[self.length - 1]; }

    T& operator[](ptrdiff_t i) { bounds_check(i, length); return data[i]; }
    sxi::span<T> span() { return { data, length }; }
};

struct string {
    union {
        struct {
            char* data_;
            int capacity_;
            int length_;
        };
        struct {
            char buffer_[12];
            int length2_; // same as length above (but sort of UB)
        };
    };

    char* data() { return (length_ < 12) ? buffer_ : data_; }
    int length() { return length_; }

    auto begin(this auto&& self) { return self.data(); }
    auto end(this auto&& self) { return self.data() + self.length(); }

    void push(char c) {
        if (length_ < 11) {
            buffer_[length_++] = c;
        } else if (length_ == 11) {
            char* ptr = alloc<char>(16);
            memcpy(ptr, buffer_, 12);
            ptr[11] = c;
            ptr[12] = 0;
            data_ = ptr;
            capacity_ = 16;
            length_ = 12;
        } else if (length_ + 1 < capacity_) {
            data_[length_++] = c;
            data_[length_] = 0;
        } else {
            data_ = realloc_x2(data_, capacity_);
            data_[length_++] = c;
            data_[length_] = 0;
        }
    }

    void dealloc() {
        if (length_ >= 12)
            free(data_, capacity_);
        *this = {};
    }

    void append(const char* data, int length) {
        if (length < 0)
            error_f("string length must be positive");
        auto new_length = length_ + length;
        if (new_length < 12) {
            memcpy(buffer_ + length_, data, length);
            buffer_[new_length] = 0;
            length_ = new_length;
        } else if (length_ < 12) {
            auto ptr = alloc<char>(new_length + 1);
            memcpy(ptr, buffer_, length_);
            memcpy(ptr + length_, data, length);
            ptr[new_length] = 0;
            data_ = ptr;
            length_ = new_length;
            capacity_ = new_length + 1;
        } else if (new_length + 1 < capacity_) {
            memcpy(data_ + length_, data, length);
            data_[new_length] = 0;
            length_ = new_length;
        } else {
            auto new_cap = capacity_ + (length_ > length ? length_ : length);
            data_ = realloc(data_, capacity_, new_cap);
            memcpy(data_ + length_, data, length);
            data_[new_length] = 0;
            length_ = new_length;
            capacity_ = new_cap;
        }
    }

    char& operator[](ptrdiff_t i) { bounds_check(i, length_); return data()[i]; }
    sxi::span<char> span() { return { data(), length_ }; }
};
static_assert(sizeof(string) == 16);

template<typename K, typename V, auto hash_fn>
struct insert_only_map {
    struct Entry { K key; V value; };

    struct LookupResult { bool present; int loc; };

    vector<Entry> items;
    int* index;
    unsigned index_length;

    LookupResult lookup(K key) const { return lookup(key, hash_fn(key)); }

    LookupResult lookup(K key, uint32_t hash) const {
        if (index) {
            auto mask = index_length - 1;
            for (unsigned i=0;; i++) {
                unsigned slot = mask & (hash + (i * (i + 1)) / 2);
                auto idx = index[slot];
                if (idx == -1)
                    return { false, (int)slot };
                if (items.data[idx].key == key)
                    return { true, idx };
            }
        } else {
            int idx = 0;
            for (auto& entry : items) {
                if (entry.key == key)
                    return { true, idx };
                idx++;
            }
            return { false, 0 };
        }
    }

    void append(K key, V value, int slot) {
        auto idx = items.length;
        items.push({key, value});
        if (index) {
            index[slot] = idx;
            if ((unsigned)idx * 3 > index_length * 2)
                reindex();
        } else if (idx > 4) {
            reindex();
        }
    }

    void reindex() {
        unsigned new_len = 1u << (33 - __builtin_clz((unsigned)items.length));
        index = sxi::realloc(index, index_length, new_len);
        index_length = new_len;

        memset(index, -1, index_length * sizeof(int));
        auto mask = index_length - 1;

        for (int idx=0; idx < items.length; idx++) {
            K key = items[idx].key;
            uint32_t hash = hash_fn(key);
            for (unsigned i=0;; i++) {
                unsigned slot = mask & (hash + (i * (i + 1)) / 2);
                if (index[slot] == -1) {
                    index[slot] = idx;
                    break;
                }
            }
        }
    }

    void dealloc() {
        sxi::free(index, index_length);
        items.dealloc();
    }
};

} // sxi

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
