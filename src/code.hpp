#pragma once

#include "sxi.hpp"
#include "utils.hpp"

namespace sxi {

enum opcode : uint16_t {
    op_literal,
    op_ret,
};

struct Code {
    vector<opcode> instructions;
    vector<Symbol*> symbols;
    vector<SXI> literals;
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

} // sxi
