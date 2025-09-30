#include "builtin.hpp"
#include "sxi.hpp"
#include "../error.hpp"

using namespace sxi;

static SXI eqp(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 2);
    return wrap_bool(argv[0] == argv[1]);
}

static SXI equalp(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 2);
    return wrap_bool(equal(argv[0], argv[1]));
}

static SXI boolp(SXI s) {
    return wrap_bool(s == c_false || s == c_true);
}

static SXI not_(SXI s) {
    return wrap_bool(s == c_false);
}

static SXI boolean(SXI s) {
    return wrap_bool(is_truthy(s));
}

static SXI boolean_eqp(int argc, SXI* argv) {
    if (argc == 0)
        return c_true;

    auto unwrap = [=](SXI s) {
        if (s == c_true) return true;
        if (s == c_false) return false;
        invalid_arguments("boolean=?", span(argv, argc));
    };

    bool val = unwrap(argv[0]);
    for (auto v : span(argv+1, argc-1))
        if (unwrap(v) != val)
            return c_false;
    return c_true;
}

static const function_1_def def1s[] = {
    { "boolean", boolean },
    { "boolean?", boolp },
    { "not", not_ },
};

static const function_n_def defns[] = {
    { "eq?", eqp },
    { "eqv?", eqp },
    { "equal?", equalp },
    { "boolean=?", boolean_eqp },
};

const builtin_lib sxi::builtin_lib_bool(def1s, defns);
