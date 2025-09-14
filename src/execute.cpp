#include "gc.hpp"
#include "sxi.hpp"
#include "code.hpp"
#include "alloc.hpp"
#include "env.hpp"

using namespace sxi;

struct Continuation {
    Code* code;
    int ip;
    Continuation* next;
    Environment* env;
};
template <> struct tt::traits<Continuation> : tt::boxed {};

#define OP(op) _label_ ## op
#define NEXT goto* jump_table[code->insns[ip]]
#define J(op) [op] = &&OP(op)
#define UNIMPLEMENTED(op) [op] = &&OP(unimplemented)

static __attribute__((noinline))
SXI execute_(Code* code, Continuation* cont, Environment* env) {
    SXI tos = c_void;
    int ip = 0;
    (void)env;

    static constexpr void* jump_table[] = {
        J(op_literal),
        J(op_ret),
        J(op_lookup),
        UNIMPLEMENTED(op_alloc_cont),
        UNIMPLEMENTED(op_alloc_stack),
        UNIMPLEMENTED(op_push),
        UNIMPLEMENTED(op_call),
        UNIMPLEMENTED(op_tailcall),
        J(op_exit),
        J(op_branch),
        J(op_branch0),
        J(op_define),
        J(op_set),
    };

    NEXT;

OP(op_literal):
    tos = code->literals[code->insns[ip+1]];
    ip += 2;
    NEXT;

OP(op_lookup):
    tos = env->lookup(code->symbols[code->insns[ip+1]]);
    ip += 2;
    NEXT;

OP(op_define):
    env->define(code->symbols[code->insns[ip+1]], tos);
    ip += 2;
    NEXT;

OP(op_set):
    env->set(code->symbols[code->insns[ip+1]], tos);
    ip += 2;
    NEXT;

OP(op_ret):
    code = cont->code;
    ip = cont->ip;
    env = cont->env;
    cont = cont->next;
    NEXT;

OP(op_exit):
    return tos;

OP(op_branch):
    ip += code->insns[ip+1];
    NEXT;

OP(op_branch0):
    ip += is_truthy(tos) ? 2 : code->insns[ip+1];
    NEXT;

OP(unimplemented):
    error_f("execute error: opcode '%s' not implemented", get_opcode_name(code->insns[ip]));
}

SXI sxi::execute(Thunk* t) {
    auto exit_insns = alloc<opcode>(1);
    exit_insns[0] = op_exit;

    auto exit_fn = gc_alloc<Code>();
    *exit_fn = {};
    exit_fn->insns = exit_insns;
    exit_fn->insns_len = 1;

    auto exit_cont = gc_alloc<Continuation>();
    *exit_cont = {};
    exit_cont->code = exit_fn;

    return execute_(t->code, exit_cont, t->env);
}