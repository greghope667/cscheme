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
#define NEXT goto* jump_table[*ip]
#define J(op) [op] = &&OP(op)
#define UNIMPLEMENTED(op) [op] = &&OP(unimplemented)

static __attribute__((noinline))
SXI execute_(Code* code, Continuation* cont, Environment* env) {
    SXI tos = c_void;
    opcode* ip = code->insns;
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
        J(op_unlambda),
    };

    NEXT;

OP(op_unlambda):
    env = env->parent;
    ip += 1;
    NEXT;

OP(op_literal):
    tos = code->literals[ip[1]];
    ip += 2;
    NEXT;

OP(op_lookup):
    tos = env->lookup(code->symbols[ip[1]]);
    ip += 2;
    NEXT;

OP(op_define):
    env->define(code->symbols[ip[1]], tos);
    ip += 2;
    NEXT;

OP(op_set):
    env->set(code->symbols[ip[1]], tos);
    ip += 2;
    NEXT;

OP(op_push):
    assert(stack->length < stack->capacity);
    stack->data[stack->length++] = tos;
    // stack->push(tos);
    ip += 1;
    NEXT;

OP(op_alloc_cont): {
        auto new_cont = gc_alloc<Continuation>();
        *new_cont = {
            .code = code,
            .ip = nullptr,
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
    stack->reserve(ip[1]);
    ip += 2;
    NEXT;

OP(op_call):
    cont->ip = ip + 1;
    goto OP(op_tailcall);

OP(op_tailcall): {
        assert(stack->length > 0);
        auto function = stack->data[0];
        auto args = span<SXI>(stack->data + 1, stack->length - 1);
        match(function) {
            case_val(Function_n, f) {
                tos = f(args.length, args.data);
                goto OP(op_ret);
            }
            case_val(Function_1, f) {
                if (args.length != 1)
                    invalid_arguments(function, args);
                tos = f(args[0]);
                goto OP(op_ret);
            }
            case_val(Function_s, f) {
                switch (f) {
                    case SXI_FUNC_apply: {
                        if (args.length < 2)
                            invalid_arguments(function, args);
                        auto tail = stack->data[stack->length-1];
                        memmove(stack->data, stack->data+1, (stack->length-2)*sizeof(SXI));
                        stack->length -= 2;
                        while (tail != c_null) {
                            stack->push(car(tail));
                            tail = cdr(tail);
                        }
                        goto OP(op_tailcall);
                    }
                    case SXI_FUNC_current_env: {
                        if (args.length > 0)
                            invalid_arguments(function, args);
                        tos = wrap(env);
                        goto OP(op_ret);
                    }
                    case SXI_FUNC_values: {
                        if (cont->ip[0] != op_push) {
                            if (args.length != 1)
                                error("cannot return values to single-valued continuation");
                            tos = args[0];
                            goto OP(op_ret);
                        }
                        cont->ip++;
                        cont->stack->reserve(cont->stack->capacity + args.length);
                        memcpy(
                            &cont->stack->data[cont->stack->length],
                            args.data,
                            args.length * sizeof(SXI)
                        );
                        cont->stack->length += args.length;
                        goto OP(op_ret);
                    }
                    default:
                        SXI_UNREACHABLE;
                }
            }
            case_lambda(l) {
                env = bind_lambda(l, args);
                code = l->code;
                ip = code->insns;
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
    ip += ip[1];
    NEXT;

OP(op_branch0):
    ip += is_truthy(tos) ? 2 : ip[1];
    NEXT;

OP(op_lambda): {
        auto pl = code->lambdas[ip[1]];
        auto lambda = gc_alloc<Lambda>();
        *lambda = { pl.code, pl.arguments, env };
        tos = wrap(lambda);
        ip += 2;
        NEXT;
    }

[[maybe_unused]] OP(unimplemented):
    error_f("execute error: opcode '%s' not implemented", get_opcode_name(ip[0]));
}

static Continuation* make_exit_cont() {
    auto exit_insns = alloc<opcode>(1);
    exit_insns[0] = op_exit;

    auto exit_fn = gc_alloc<Code>();
    *exit_fn = {};
    exit_fn->insns = exit_insns;
    exit_fn->insns_len = 1;

    auto exit_cont = gc_alloc<Continuation>();
    *exit_cont = {};
    exit_cont->ip = exit_fn->insns;
    exit_cont->code = exit_fn;

    return exit_cont;
}

SXI sxi::call(Lambda* l, int argc, SXI* argv) {
    return execute_(l->code, make_exit_cont(), bind_lambda(l, span(argv, argc)));
}
