#include "builtin.hpp"
#include "sxi.hpp"

#include <limits.h>

using namespace sxi;

static SXI plus(int argc, SXI* argv) {
    sxi_int total = 0;
    for (auto v : span(argv, argc))
        total += as<sxi_int>(v);
    return wrap(total);
}

static SXI minus(int argc, SXI* argv) {
    if (argc == 2)
        return wrap(as<sxi_int>(argv[0]) - as<sxi_int>(argv[1]));
    if (argc == 1)
        return wrap(-as<sxi_int>(argv[0]));
    SXI_CHECK_ARITY(1, INT_MAX);

    auto total = as<sxi_int>(argv[0]);
    for (auto v : span(argv+1, argc-1))
        total -= as<sxi_int>(v);
    return wrap(total);
}

template <typename Compare>
static bool compare(int argc, SXI* argv, Compare comp) {
    if (argc == 2)
        return comp(as<sxi_int>(argv[0]), as<sxi_int>(argv[1]));
    SXI_CHECK_ARITY(2, INT_MAX);

    auto current = as<sxi_int>(argv[0]);
    for (auto v : span(argv+1, argc-1)) {
        auto i = as<sxi_int>(v);
        if (not comp(current, i))
            return false;
        current = i;
    }
    return true;
}

static SXI less(int argc, SXI* argv) {
    return wrap_bool(compare(argc, argv, [](sxi_int a, sxi_int b) { return a < b; }));
}

static SXI greater(int argc, SXI* argv) {
    return wrap_bool(compare(argc, argv, [](sxi_int a, sxi_int b) { return a > b; }));
}

static SXI equal(int argc, SXI* argv) {
    return wrap_bool(compare(argc, argv, [](sxi_int a, sxi_int b) { return a == b; }));
}

struct divide_op_result { sxi_int a, b; };
static divide_op_result divide_op(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 2);
    auto a = as<sxi_int>(argv[0]);
    auto b = as<sxi_int>(argv[1]);
    if (b == 0)
        error("divide by zero");
    return { a, b };
}

static SXI remainder(int argc, SXI* argv) {
    auto [num, div] = divide_op(argc, argv);
    return wrap(num % div);
}

static const function_1_def def1s[] = {
    { "integer?", [](SXI s) { return wrap_bool(instance<sxi_int>(s)); } },
};

static const function_n_def defns[] = {
    { "+", plus },
    { "-", minus },
    { "<", less },
    { ">", greater },
    { "=", equal },
    { "remainder", remainder },
};

const builtin_lib sxi::builtin_lib_number(def1s, defns);
