#include "builtin.hpp"
#include "sxi.hpp"

#include <limits.h>

using namespace sxi;

static SXI cons(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 2);
    return wrap(new Pair{ argv[0], argv[1] });
}

static SXI xcons(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 2);
    return wrap(new Pair{ argv[1], argv[0] });
}

static SXI set_car(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 2);
    as<Pair>(argv[0])->first = argv[1];
    return argv[0];
}

static SXI set_cdr(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 2);
    as<Pair>(argv[0])->second = argv[1];
    return argv[0];
}

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

struct list_builder_fwd {
    SXI head;
    SXI* tail;
    list_builder_fwd() : head(), tail(&head) {}
    void push(SXI x) {
        auto p = new Pair;
        p->first = x;
        *tail = wrap(p);
        tail = &p->second;
    }
    SXI end(SXI value = c_null) {
        *tail = value;
        return head;
    }
};

static SXI list_p(SXI x) {
    for (;;) {
        if (x == c_null) return c_true;
        if (not instance<Pair>(x))
            return c_false;
        x = cdr(x);
    }
}

static SXI list_length(SXI x) {
    ptrdiff_t length = 0;
    for (auto _ : listerate(x)) length++;
    return wrap(length);
}

static SXI list(int argc, SXI* argv) {
    SXI head = c_null;
    while (argc --> 0)
        head = cons(argv[argc], head);
    return head;
}

static SXI make_list(int argc, SXI* argv) {
    SXI_CHECK_ARITY(1, 2);
    auto length = as<integer>(argv[0]);
    auto fill = argc == 2 ? argv[1] : c_void;
    if (length < 0)
        error("negative make-list length");
    SXI head = c_null;
    while (length --> 0)
        head = cons(fill, head);
    return head;
}

static SXI list_reverse(SXI x) {
    SXI head = c_null;
    for (auto v : listerate(x))
        head = cons(v, head);
    return head;
}

static SXI list_tail(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 2);
    auto length = as<integer>(argv[1]);
    if (length <= 0)
        return argv[0];
    auto it = listerate(argv[0]).begin();
    for (auto end = listerate::sentinel(); it != end; ++it) {
        if (length-- == 0)
            return it.value;
    }
    invalid_arguments(__FUNCTION__, span(argv, argc));
}

static SXI list_ref(int argc, SXI* argv) {
    return car(list_tail(argc, argv));
}

static SXI general_car_cdr(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 2);
    auto object = argv[0];
    auto path = as<integer>(argv[1]);
    while (path > 1) {
        object = (path & 1) ? car(object) : cdr(object);
        path >>= 1;
    }
    return object;
}

static SXI list_append(int argc, SXI* argv) {
    if (argc == 0) return c_null;
    list_builder_fwd l;
    for (int i=0; i<argc-1; i++) {
        for (auto x : listerate(argv[i]))
            l.push(x);
    }
    return l.end(argv[argc-1]);
}

static SXI list_set(int argc, SXI* argv) {
    SXI_CHECK_ARITY(3, 3);
    as<Pair>(list_tail(2, argv))->first = argv[2];
    return argv[0];
}

static SXI list_copy(SXI l) {
    list_builder_fwd copy;
    for (auto x : listerate(l))
        copy.push(x);
    return copy.end();
}

static SXI listcons(int argc, SXI* argv) {
    SXI_CHECK_ARITY(1, INT_MAX);
    SXI head = argv[--argc];
    while (argc --> 0)
        head = cons(argv[argc], head);
    return head;
}

static const function_1_def def1s[] = {
    { "pair?", [](SXI x) { return wrap_bool(instance<Pair>(x)); }},
    { "car", car },
    { "cdr", cdr },
    { "null?", [](SXI x) { return wrap_bool(x == c_null); }},
    { "list?", list_p },
    { "length", list_length },
    { "list-length", list_length },
    { "reverse", list_reverse },
    { "list-reverse", list_reverse },
    { "list-copy", list_copy },
};

static const function_n_def defns[] = {
    { "cons", cons },
    { "xcons", xcons },
    { "list*", listcons },
    { "cons*", listcons },
    { "set-car!", set_car },
    { "set-cdr!", set_cdr },
    { "list", list },
    { "make-list", make_list },
    { "list-tail", list_tail },
    { "list-ref", list_ref },
    { "general-car-cdr", general_car_cdr },
    { "append", list_append },
    { "list-append", list_append },
    { "list-set!", list_set },
};

const builtin_lib sxi::builtin_lib_list(def1s, defns);
