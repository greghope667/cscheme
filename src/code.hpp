#pragma once

#include "sxi.hpp"
#include "tl.hpp"

namespace sxi {

enum opcode : int16_t {
    op_literal,
    op_ret,
    op_lookup,
    op_alloc_cont,
    op_alloc_stack,
    op_push,
    op_call,
    op_tailcall,
    op_exit,
    op_branch,
    op_branch0,
    op_define,
    op_set,
};

const char* get_opcode_name(opcode op);

struct CompiledLambda;

struct Code {
    opcode* insns;
    Symbol** symbols;
    SXI* literals;
    CompiledLambda* lambdas;
    int16_t insns_len;
    int16_t symbols_len;
    int16_t literals_len;
    int16_t lambdas_len;
};
template <> struct tt::traits<Code> : tt::boxed {};

struct Formals {
    vector<Symbol*> names;
    bool is_variadic;
};
template <> struct tt::traits<Formals> : tt::boxed {};

struct Lambda {
    Code* code;
    Formals* arguments;
    Environment* capture;
};

struct Thunk {
    Code* code;
    Environment* env;
};

struct CompiledLambda {
    Code* code;
    Formals* arguments;
};

} // sxi
