#pragma once

#include "sxi.hpp"
#include "tl.hpp"

namespace sxi {

#define OPCODES \
    X(op_literal) \
    X(op_ret) \
    X(op_lookup) \
    X(op_alloc_cont) \
    X(op_alloc_stack) \
    X(op_push) \
    X(op_call) \
    X(op_tailcall) \
    X(op_exit) \
    X(op_branch) \
    X(op_branch0) \
    X(op_define) \
    X(op_set) \
    X(op_lambda) \
    X(op_unlambda) \

#define X(op) +1
constexpr int OPCODE_MAX = (OPCODES);
#undef X

enum opcode : int16_t {
    #define X(op) op ,
    OPCODES
    #undef X
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
    Symbol* name;
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

struct ProtoLambda {
    Code* code;
    Formals* arguments;
};

enum Function_s { SXI_FUNC_apply, SXI_FUNC_current_env, SXI_FUNC_values };
template <> struct tt::traits<Function_s> : tt::tagged<SXI_TAG_function_s>, tt::unboxed {};

struct Continuation {
    Continuation* next;
};
template <> struct tt::traits<Continuation> : tt::boxed {};

struct Fiber {
    struct ReturnFrame {
        Code* code;
        opcode* ip;
        Environment* env;
        int args_begin;
        int args_end;
    };

    vector<ReturnFrame> frames;
    vector<SXI> stack;
    Environment* globals;
    SXI tos;
    ReturnFrame* cont;
};

} // sxi
