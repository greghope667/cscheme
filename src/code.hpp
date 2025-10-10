#pragma once

#include "sxi.hpp"
#include "tl.hpp"
#include "vector.hpp"

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
    op_lambda,
};

const char* get_opcode_name(opcode op);

struct ProtoLambda;

struct Code {
    opcode* insns;
    Symbol** symbols;
    SXI* literals;
    ProtoLambda* lambdas;
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

struct Chunk {
    Code* code;
    Environment* env;
};

struct ProtoLambda {
    Code* code;
    Formals* arguments;
};

enum Function_s { SXI_FUNC_apply, SXI_FUNC_current_env, SXI_FUNC_values };
template <> struct tt::traits<Function_s> : tt::tagged<SXI_TAG_function_s>, tt::unboxed {};

struct Continuation {
    Code* code;
    opcode* ip;
    Continuation* next;
    Environment* env;
    Vector* stack;
};
template <> struct tt::traits<Continuation> : tt::boxed {};

} // sxi
