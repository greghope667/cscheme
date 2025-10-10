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

static SXI gensym(int argc, SXI* argv) {
    SXI_CHECK_ARITY(0, 0);
    return wrap(sxi::gensym());
}

static SXI compile(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 2);
    return wrap(sxi::compile(argv[0], as<Environment>(argv[1])));
}

static SXI disassemble(SXI value) {
    sxi::disassemble(stdout, as<Lambda>(value));
    return c_void;
}

static const function_1_def def1s[] = {
    { "display", display },
    { "write", display },
    { "disassemble", disassemble },
};

static const function_n_def defns[] = {
    { "newline", newline },
    { "gensym", gensym },
    { "compile", compile },
};

const builtin_lib sxi::builtin_lib_misc(def1s, defns);
