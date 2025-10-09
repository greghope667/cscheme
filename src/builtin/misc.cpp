#include "builtin.hpp"

using namespace sxi;

static SXI display(SXI value) {
    print(stdout, value);
    return value;
}

static SXI newline(int argc, SXI* argv) {
    SXI_CHECK_ARITY(0, 0);
    putchar('\n');
    return c_void;
}

static const function_1_def def1s[] = {
    { "display", display },
    { "write", display },
};

static const function_n_def defns[] = {
    { "newline", newline },
};

const builtin_lib sxi::builtin_lib_misc(def1s, defns);
