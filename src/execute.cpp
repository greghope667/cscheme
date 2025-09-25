#include "gc.hpp"
#include "match.hpp"
#include "sxi.hpp"
#include "code.hpp"
#include "alloc.hpp"
#include "env.hpp"
#include "vector.hpp"
#include "error.hpp"

#include "assert.h"
#include "limits.h"

using namespace sxi;

static Environment*
bind_lambda(Lambda* l, span<SXI> args) {
    if (l->arguments->is_variadic)
        error("variadics not implemented");

    int len = l->arguments->names.length;

    if (len != args.length)
        invalid_arguments(wrap(l), args);

    auto env = gc_alloc<Environment>();
    *env = { .parent = l->capture, .table = {} };
    env->table.items.reserve(len);
    env->table.items.length = len;

    for (int i=0; i<len; i++) {
        env->table.items[i] = {
            .key = l->arguments->names[i],
            .value = args[i]
        };
    }
    return env;
}

#define OP(op) _label_ ## op
#define NEXT goto* jump_table[code->insns[ip]]
#define J(op) [op] = &&OP(op)
#define UNIMPLEMENTED(op) [op] = &&OP(unimplemented)

static __attribute__((noinline))
SXI execute_(Code* code, Continuation* cont, Environment* env) {
    SXI tos = c_void;
    int ip = 0;
    Vector* stack = nullptr;

    static constexpr void* jump_table[] = {
        J(op_literal),
        J(op_ret),
        J(op_lookup),
        J(op_alloc_cont),
        J(op_alloc_stack),
        J(op_push),
        J(op_call),
        J(op_tailcall),
        J(op_exit),
        J(op_branch),
        J(op_branch0),
        J(op_define),
        J(op_set),
        J(op_lambda),
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

OP(op_push):
    assert(stack->length < stack->capacity);
    stack->push(tos);
    ip += 1;
    NEXT;

OP(op_alloc_cont): {
        auto new_cont = gc_alloc<Continuation>();
        *new_cont = {
            .code = code,
            .ip = INT_MAX,
            .next = cont,
            .env = env,
            .stack = stack,
        };
        cont = new_cont;
        stack = nullptr;
    }
    ip += 1;
    NEXT;

OP(op_alloc_stack):
    stack = gc_alloc<Vector>();
    *stack = {};
    stack->reserve(code->insns[ip+1]);
    ip += 2;
    NEXT;

OP(op_call):
    cont->ip = ip + 1;
    goto* &&OP(op_tailcall);

OP(op_tailcall): {
        assert(stack->length > 0);
        auto function = stack->data[0];
        auto args = span<SXI>(stack->data + 1, stack->length - 1);
        match(function) {
            case_val(Function_n, f) {
                tos = f(args.length, args.data);
                goto* &&OP(op_ret);
            }
            case_val(Function_1, f) {
                if (args.length != 1)
                    invalid_arguments(function, args);
                tos = f(args[0]);
                goto* &&OP(op_ret);
            }
            case_lambda(l) {
                env = bind_lambda(l, args);
                code = l->code;
                ip = 0;
                NEXT;
            }
            default:
                error(function, "object is not callable");
        }
    }

OP(op_ret):
    if (gc_allocations > 4096)
        gc_run(cont, tos);
    code = cont->code;
    ip = cont->ip;
    env = cont->env;
    stack = cont->stack;

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

OP(op_lambda): {
        auto pl = code->lambdas[code->insns[ip+1]];
        auto lambda = gc_alloc<Lambda>();
        *lambda = { pl.code, pl.arguments, env };
        tos = wrap(lambda);
        ip += 2;
        NEXT;
    }

[[maybe_unused]] OP(unimplemented):
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
