#include "builtin.hpp"
#include "sxi.hpp"
#include "../error.hpp"

using namespace sxi;

static SXI cons(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 2);
    return cons(argv[0], argv[1]);
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
    auto length = as<sxi_int>(argv[0]);
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
    auto length = as<sxi_int>(argv[1]);
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
};

static const function_n_def defns[] = {
    { "cons", cons },
    { "set-car!", set_car },
    { "set-cdr!", set_cdr },
    { "list", list },
    { "make-list", make_list },
    { "list-tail", list_tail },
    { "list-ref", list_ref },
};

const builtin_lib sxi::builtin_lib_list(def1s, defns);
