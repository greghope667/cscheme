#include "builtin.hpp"
#include "../env.hpp"

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

static SXI env_ref(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 3);
    if (argc == 2) {
        return as<Environment>(argv[0])->lookup(as<Symbol>(argv[1]));
    } else {
        SXI v = argv[2];
        (void)as<Environment>(argv[0])->try_lookup(as<Symbol>(argv[1]), &v);
        return v;
    }
}

static const function_1_def def1s[] = {
    { "symbol?", [](SXI s) { return wrap_bool(instance<Symbol>(s)); } },
    { "display", display },
    { "write", display },
    { "disassemble", disassemble },
    { "exit", [](SXI s) { exit(instance<sxi_int>(s) ? as<sxi_int>(s) : 1); return c_void; } },
};

static const function_n_def defns[] = {
    { "newline", newline },
    { "gensym", gensym },
    { "compile", compile },
    { "env-ref", env_ref },
};

const builtin_lib sxi::builtin_lib_misc(def1s, defns);
