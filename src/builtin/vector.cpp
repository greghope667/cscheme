#include "builtin.hpp"
#include "sxi.hpp"
#include "../vector.hpp"
#include "../gc.hpp"

#include <limits.h>

using namespace sxi;

static SXI make_vector(int argc, SXI* argv) {
    SXI_CHECK_ARITY(1, 2);

    auto len = as<sxi_int>(argv[0]);
    if (len > INT_MAX || len < 0)
        error_f("invalid length for make-vector: %zi", len);

    auto fill = argc == 2 ? argv[1] : c_void;
    auto vec = make_vector(len);
    for (auto& v : *vec)
        v = fill;

    return wrap(vec);
}

static SXI vector(int argc, SXI* argv) {
    auto v = gc_alloc<Vector>();
    *v = {};
    v->reserve(argc);
    memcpy(v->data, argv, argc * sizeof(SXI));
    v->length = argc;
    return wrap(v);
}

static SXI vector_length(SXI vec) {
    return wrap<sxi_int>(as<Vector>(vec)->length);
}

static SXI vector_ref(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 2);
    return (*as<Vector>(argv[0]))[as<sxi_int>(argv[1])];
}

static SXI vector_set(int argc, SXI* argv) {
    SXI_CHECK_ARITY(3, 3);
    auto v = as<Vector>(argv[0]);
    (*v)[as<sxi_int>(argv[1])] = argv[2];
    return wrap(v);
}

static SXI vector_to_list(int argc, SXI* argv) {
    SXI_CHECK_ARITY(1, 3);
    auto vec = as<Vector>(argv[0]);
    auto begin = argc >= 2 ? as<sxi_int>(argv[1]) : 0;
    auto end = argc == 3 ? as<sxi_int>(argv[2]) : vec->length;
    if (begin < 0 || end < begin || vec->length < end)
        invalid_arguments(__FUNCTION__, span(argv, argc));

    auto list = c_null;
    for (auto i=end; i --> begin; )
        list = cons(vec->data[i], list);
    return list;
}

// TODO: move to common header
struct listerate {
    struct sentinel {};
    struct iterator {
        SXI value;
        iterator& operator++() {
            value = cdr(value);
            return *this;
        }
        SXI operator*() { return car(value); }
        bool operator==(sentinel) { return value == c_null; }
    } it;

    listerate(SXI value) : it(value) {}

    auto begin() { return it; }
    auto end() { return sentinel{}; }
};

static SXI list_to_vector(SXI list) {
    auto v = gc_alloc<Vector>();
    *v = {};
    for (auto x : listerate(list))
        v->push(x);
    v->shrink_to_fit();
    return wrap(v);
}

static SXI vector_copy(int argc, SXI* argv) {
    SXI_CHECK_ARITY(1, 3);
    auto vec = as<Vector>(argv[0]);
    auto begin = argc >= 2 ? as<sxi_int>(argv[1]) : 0;
    auto end = argc == 3 ? as<sxi_int>(argv[2]) : vec->length;
    if (begin < 0 || end < begin || vec->length < end)
        invalid_arguments(__FUNCTION__, span(argv, argc));

    return ::vector(end - begin, vec->data + begin);
}

static SXI vector_copy_mut(int argc, SXI* argv) {
    SXI_CHECK_ARITY(3, 5);
    auto to = as<Vector>(argv[0]);
    auto at = as<sxi_int>(argv[1]);
    auto from = as<Vector>(argv[2]);
    auto start = argc >= 4 ? as<sxi_int>(argv[3]) : 0;
    auto end = argc == 5 ? as<sxi_int>(argv[4]) : from->length;

    if (at < 0 || to->length - at < end - start ||
        start < 0 || end < start || from->length < end
    )
        invalid_arguments(__FUNCTION__, span(argv, argc));

    memmove(to->data + at, from->data + start, (end - start) * sizeof(SXI));
    return argv[0];
}

static SXI vector_append(int argc, SXI* argv) {
    sxi_int length = 0;
    for (auto v : span(argv, argc))
        length += as<Vector>(v)->length;

    auto vec = gc_alloc<Vector>();
    *vec = {};
    vec->reserve(length);

    for (auto v : span(argv, argc)) {
        auto src = as<Vector>(v);
        memcpy(vec->data + vec->length, src->data, src->length * sizeof(SXI));
        vec->length += src->length;
    }

    return wrap(vec);
}

static SXI vector_fill(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 4);
    auto vec = as<Vector>(argv[0]);
    auto fill = argv[1];
    auto begin = argc >= 3 ? as<sxi_int>(argv[2]) : 0;
    auto end = argc == 4 ? as<sxi_int>(argv[3]) : vec->length;
    if (begin < 0 || end < begin || vec->length < end)
        invalid_arguments(__FUNCTION__, span(argv, argc));

    for (auto& v : span(vec->data + begin, end - begin))
        v = fill;

    return argv[0];
}

static SXI vector_append_mut(int argc, SXI* argv) {
    SXI_CHECK_ARITY(1, INT_MAX);
    auto vec = as<Vector>(argv[0]);
    auto length = vec->length;
    auto rest = span(argv+1, argc-1);

    for (auto v : rest)
        length += as<Vector>(v)->length;

    vec->reserve(length);

    for (auto v : rest) {
        auto src = as<Vector>(v);
        memcpy(vec->data + vec->length, src->data, src->length * sizeof(SXI));
        vec->length += src->length;
    }

    return argv[0];
}

static SXI vector_push(int argc, SXI* argv) {
    SXI_CHECK_ARITY(1, INT_MAX);
    auto vec = as<Vector>(argv[0]);
    for (auto x : span(argv+1, argc-1))
        vec->push(x);
    return argv[0];
}

static SXI vector_pop(SXI v) {
    auto vec = as<Vector>(v);
    auto x = vec->back();
    vec->length--;
    return x;
}

static const function_1_def def1s[] = {
    { "vector?", [](SXI s) { return wrap_bool(instance<Vector>(s)); } },
    { "vector-length", vector_length },
    { "list->vector", list_to_vector },
    { "vector-pop!", vector_pop },
};

static const function_n_def defns[] = {
    { "make-vector", make_vector },
    { "vector", ::vector },
    { "vector-ref", vector_ref },
    { "vector-set!", vector_set },
    { "vector->list", vector_to_list },
    { "vector-copy", vector_copy },
    { "vector-copy!", vector_copy_mut },
    { "vector-append", vector_append },
    { "vector-fill!", vector_fill },
    { "vector-append!", vector_append_mut },
    { "vector-push!", vector_push },
};

const builtin_lib sxi::builtin_lib_vector(def1s, defns);
