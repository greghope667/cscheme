#include "gc.hpp"
#include "code.hpp"
#include "error.hpp"
#include "match.hpp"
#include "sxi.hpp"
#include "env.hpp"
#include "vector.hpp"
#include "string.hpp"
#include "struct.hpp"
#include <assert.h>

using namespace sxi;

/// 4K blocks (from malloc) to use as pools for objects

#define BLOCK_SIZE 4096zu

static void* alloc_block() {
    static constexpr size_t BLOCK_ALLOC_COUNT = 4;
    static char* blocks;
    static int block_count;

    if (block_count == 0) {
        blocks = (char*)aligned_alloc(BLOCK_SIZE, BLOCK_ALLOC_COUNT * BLOCK_SIZE);
        if (not blocks)
            error("out of memory");
        block_count = BLOCK_ALLOC_COUNT;
    }

    blocks = blocks + BLOCK_SIZE;
    block_count--;
    return blocks - BLOCK_SIZE;
}

/// All types subject to GC

#define GC_TAG_TYPES \
    X(Environment) \
    X(Lambda) \
    X(Pair) \
    X(Vector) \
    X(String) \
    X(StructType) \
    X(StructInstance) \

#define GC_TYPES \
    GC_TAG_TYPES \
    X(Code) \
    X(Continuation) \
    X(Formals) \

/// Recursive mark/sweep functions

static void mark(SXI value);
template <typename T> static void mark(T*);
template <typename T> static void mark_children(T*);
template <typename T> static void deallocate(T*);

template <typename T>
struct allocator {
    static constexpr size_t item_size = sizeof(T);
    static constexpr size_t N = (BLOCK_SIZE / (1 + item_size)) & ~0xfzu;

    enum state : unsigned char {
        inactive = 0,
        active,
        marked,
    };

    union entry {
        T t;
        entry* next_free;
    };

    struct block {
        state states[N];
        entry entries[N];

        T* alloc(T* x) {
            auto offset = reinterpret_cast<entry*>(x) - entries;
            assert(0 <= offset && (size_t)offset < N);
            assert(states[offset] == inactive);
            states[offset] = active;
            gc_allocations++;
            return x;
        }

        void mark(T* x) {
            auto offset = reinterpret_cast<entry*>(x) - entries;
            assert(0 <= offset && (size_t)offset < N);
            switch (states[offset]) {
                case inactive:
                    SXI_UNREACHABLE;
                case marked:
                    break;
                case active:
                    states[offset] = marked;
                    mark_children(x);
            }
        }

        void sweep(entry** frees) {
            for (size_t offset=0; offset<N; offset++) {
                auto* entry = &entries[offset];
                switch (states[offset]) {
                    case inactive:
                        break;
                    case marked:
                        states[offset] = active;
                        break;
                    case active:
                        states[offset] = inactive;
                        deallocate(&entry->t);
                        entry->next_free = *frees;
                        *frees = entry;
                        break;
                }
            }
        }
    };
    static_assert(sizeof(block) <= BLOCK_SIZE);

    static block* block_of(T* x) {
        auto addr = reinterpret_cast<uintptr_t>(x);
        return reinterpret_cast<block*>(addr & ~(BLOCK_SIZE - 1));
    }

    T* alloc() {
        if (free_list) {
            auto* x = &free_list->t;
            free_list = free_list->next_free;
            auto b = block_of(x);
            return b->alloc(x);
        }

        if (next_index < N) {
            auto b = blocks[blocks.length-1];
            auto* x = &b->entries[next_index++].t;
            return b->alloc(x);
        }

        auto b = reinterpret_cast<block*>(alloc_block());
        memset(b->states, inactive, N);
        blocks.push(b);
        next_index = 1;
        return b->alloc(&b->entries[0].t);
    }

    void mark(T* x) {
        block_of(x)->mark(x);
    }

    void sweep() {
        for (auto b : blocks) {
            b->sweep(&free_list);
        }
    }

    vector<block*> blocks = {};
    entry* free_list = {};
    size_t next_index = N;

    static allocator instance;
};

/// Explicit allocator instantiations

template <typename T> allocator<T> allocator<T>::instance = {};

template <tt::Boxed T>
T* sxi::gc_alloc() {
    return allocator<T>::instance.alloc();
}

#define X(T) template T* sxi::gc_alloc<T>();
GC_TYPES
#undef X

/// Marking

static void mark(SXI value) {
    #define X(T) case_ptr(T, v) { return allocator<T>::instance.mark(v); }
    match (value) {
        GC_TAG_TYPES
        default:;
    }
    #undef X
}

template <tt::Boxed T>
static void mark(T* x) {
    if (x)
        allocator<T>::instance.mark(x);
}

template <typename T>
static void mark_children(T*) {};

template <>
void mark_children(Pair* p) {
    mark(p->first);
    mark(p->second);
}

template <>
void mark_children(Environment* e) {
    mark(e->parent);
    for (auto [_, value] : e->table.items)
        mark(value);
}

template <>
void mark_children(Vector* v) {
    for (auto v : *v)
        mark(v);
}

template <>
void mark_children(Lambda* l) {
    mark(l->arguments);
    mark(l->capture);
    mark(l->code);
}

template <>
void mark_children(Code* c) {
    for (int i=0; i<c->lambdas_len; i++) {
        mark(c->lambdas[i].code);
        mark(c->lambdas[i].arguments);
    }
    for (int i=0; i<c->literals_len; i++)
        mark(c->literals[i]);
}

template <>
void mark_children(Continuation* c) {
    mark(c->next);
}

template <>
void mark_children(StructInstance* si) {
    mark(si->type);
    for (auto field : si->fields)
        mark(field);
}

/// Deallocation of memory from outside pool

template <typename T>
static void deallocate(T*) {};

template <>
void deallocate(Environment* e) {
    e->table.dealloc();
}

template <>
void deallocate(Vector* v) {
    v->dealloc();
}

template <>
void deallocate(Code* c) {
    free(c->insns, c->insns_len);
    free(c->lambdas, c->lambdas_len);
    free(c->symbols, c->symbols_len);
    free(c->literals, c->literals_len);
}

template <>
void deallocate(Formals* f) {
    f->names.dealloc();
}

template <>
void deallocate(String* s) {
    s->dealloc();
}

template <>
void deallocate(StructType* st) {
    st->field_names.dealloc();
}

template <>
void deallocate(StructInstance* si) {
    si->fields.dealloc();
}

/// GC module API

static vector<SXI> gc_protected = {};

void sxi::gc_protect(SXI value) {
    gc_protected.push(value);
}

void sxi::gc_unprotect(SXI value) {
    for (auto& v : gc_protected) {
        if (v == value) {
            v = gc_protected.data[--gc_protected.length];
            return;
        }
    }
    error(value, "gc_unprotect() object was not protected");
}

void sxi::gc_run(ExecStack& es, SXI tos) {
    for (auto p : gc_protected)
        mark(p);
    mark(tos);
    for (auto& frame : es.frames) {
        mark(frame.code);
        mark(frame.env);
    }
    for (auto v : es.stack)
        mark(v);

    #define X(T) allocator<T>::instance.sweep();
    GC_TYPES
    #undef X

    gc_allocations = 0;
}

/// Constructors exported in sxi.hpp

Pair* sxi::alloc_pair() {
    auto pair = gc_alloc<Pair>();
    *pair = { c_null, c_null };
    return pair;
}

sxi::Environment* sxi::make_environment(Environment* parent) {
    auto env = gc_alloc<Environment>();
    *env = { .parent = parent, .table = {} };
    return env;
}

String* sxi::make_string() {
    auto str = gc_alloc<String>();
    *str = {};
    return str;
}

String* sxi::make_string(const char* data, int len) {
    if (len < 0)
        error_f("negative string length not allowed: %i", len);
    auto str = gc_alloc<String>();
    *str = {};
    str->append(data, len);
    return str;
}

String* sxi::make_string(const char* data) {
    return make_string(data, strlen(data));
}

Vector* sxi::make_vector(int len) {
    auto vec = gc_alloc<Vector>();
    *vec = {};
    if (len > 0) {
        vec->reserve(len);
        vec->length = len;
        memset(vec->data, 0, len * sizeof(SXI));
    }
    return vec;
}
