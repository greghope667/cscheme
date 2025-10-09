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

static SXI less(int argc, SXI* argv) {
    if (argc == 2)
        return wrap_bool(as<sxi_int>(argv[0]) < as<sxi_int>(argv[1]));
    SXI_CHECK_ARITY(2, INT_MAX);

    auto largest = as<sxi_int>(argv[0]);
    for (auto v : span(argv+1, argc-1)) {
        auto i = as<sxi_int>(v);
        if (largest >= i)
            return c_false;
        largest = i;
    }
    return c_true;
}

static const function_1_def def1s[] = {
    { "integer?", [](SXI s) { return wrap_bool(instance<sxi_int>(s)); } },
};

static const function_n_def defns[] = {
    { "+", plus },
    { "-", minus },
    { "<", less },
};

const builtin_lib sxi::builtin_lib_number(def1s, defns);
