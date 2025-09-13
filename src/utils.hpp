#pragma once
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#define sxi_realloc(p, o, n) realloc(p, n)
namespace sxi { [[noreturn]] void error_f(const char*, ...); }

static inline void bounds_check(int index, int length) {
    if (index < 0 || index >= length)
        sxi::error_f("index out of range (%i > %i)", index, length);
}

template <typename T>
struct span {
    T* data;
    ptrdiff_t length;

    auto begin(this auto&& self) { return self.data; }
    auto end(this auto&& self) { return self.data + self.length; }

    T& operator[](int i) { bounds_check(i, length); return data[i]; }
};

template <typename T>
struct vector {
    T* data;
    int length, capacity;

    void push(T x) {
        if (length == capacity) {
            int new_cap = capacity < 4 ? 4 : capacity*2;
            data = (T*)sxi_realloc(data, capacity*sizeof(T), new_cap*sizeof(T));
            capacity = new_cap;
        }
        data[length++] = x;
    }

    void reserve(int count) {
        if (count > capacity) {
            data = (T*)sxi_realloc(data, capacity*sizeof(T), count*sizeof(T));
            capacity = count;
        }
    }

    auto begin(this auto&& self) { return self.data; }
    auto end(this auto&& self) { return self.data + self.length; }

    T& operator[](int i) { bounds_check(i, length); return data[i]; }
    ::span<T> span() { return { data, length }; }

    static constexpr vector empty() { return vector{}; }
};

template<typename K, typename V, auto hash_fn>
struct insert_only_map {
    struct Entry { K key; V value; };

    struct LookupResult { bool present; int loc; };

    vector<Entry> items;
    int* index;
    unsigned index_length;

    static constexpr insert_only_map empty() { return insert_only_map{}; }

    LookupResult lookup(K key) { return lookup(key, hash_fn(key)); }

    LookupResult lookup(K key, uint32_t hash) {
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
        index = (int*)sxi_realloc(index, index_length * sizeof(int), new_len * sizeof(int));
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
