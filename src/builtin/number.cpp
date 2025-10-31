#include "builtin.hpp"
#include "sxi.hpp"

#include <limits.h>

using namespace sxi;

static SXI plus(int argc, SXI* argv) {
    integer total = 0;
    for (auto v : span(argv, argc))
        total += as<integer>(v);
    return wrap(total);
}

static SXI minus(int argc, SXI* argv) {
    if (argc == 2)
        return wrap(as<integer>(argv[0]) - as<integer>(argv[1]));
    if (argc == 1)
        return wrap(-as<integer>(argv[0]));
    SXI_CHECK_ARITY(1, INT_MAX);

    auto total = as<integer>(argv[0]);
    for (auto v : span(argv+1, argc-1))
        total -= as<integer>(v);
    return wrap(total);
}

template <typename Compare>
static bool compare(int argc, SXI* argv, Compare comp) {
    if (argc == 2)
        return comp(as<integer>(argv[0]), as<integer>(argv[1]));
    if (argc < 2)
        return true;

    auto current = as<integer>(argv[0]);
    for (auto v : span(argv+1, argc-1)) {
        auto i = as<integer>(v);
        if (not comp(current, i))
            return false;
        current = i;
    }
    return true;
}

static SXI less(int argc, SXI* argv) {
    return wrap_bool(compare(argc, argv, [](integer a, integer b) { return a < b; }));
}

static SXI greater(int argc, SXI* argv) {
    return wrap_bool(compare(argc, argv, [](integer a, integer b) { return a > b; }));
}

static SXI equal(int argc, SXI* argv) {
    return wrap_bool(compare(argc, argv, [](integer a, integer b) { return a == b; }));
}

static SXI greater_eq(int argc, SXI* argv) {
    return wrap_bool(compare(argc, argv, [](integer a, integer b) { return a >= b; }));
}

static SXI less_eq(int argc, SXI* argv) {
    return wrap_bool(compare(argc, argv, [](integer a, integer b) { return a <= b; }));
}

struct divide_op_result { integer a, b; };
static divide_op_result divide_op(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 2);
    auto a = as<integer>(argv[0]);
    auto b = as<integer>(argv[1]);
    if (b == 0)
        error("divide by zero");
    return { a, b };
}

static SXI remainder(int argc, SXI* argv) {
    auto [num, div] = divide_op(argc, argv);
    return wrap(num % div);
}

static SXI times(int argc, SXI* argv) {
    integer product = 1;
    for (auto v : span(argv, argc))
        product *= as<integer>(v);
    return wrap(product);
}

static const function_1_def def1s[] = {
    { "integer?", [](SXI s) { return wrap_bool(instance<integer>(s)); } },
    { "zero?", [](SXI s) { return wrap_bool(as<integer>(s) == 0); } },
};

static const function_n_def defns[] = {
    { "+", plus },
    { "-", minus },
    { "<", less },
    { "<=", less_eq },
    { ">", greater },
    { ">=", greater_eq },
    { "=", equal },
    { "remainder", remainder },
    { "*", times },
};

const builtin_lib sxi::builtin_lib_number(def1s, defns);
