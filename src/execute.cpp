#include "gc.hpp"
#include "match.hpp"
#include "sxi.hpp"
#include "code.hpp"
#include "alloc.hpp"
#include "env.hpp"
#include "error.hpp"

#include "assert.h"
#include "limits.h"

using namespace sxi;

static Environment*
bind_lambda(Lambda* l, span<SXI> args) {
    int len = l->arguments->names.length;

    if (l->arguments->is_variadic) {
        if (len - 1 > args.length)
            invalid_arguments(wrap(l), args);
        auto variadics = c_null;
        for (int i = args.length-1; i >= len-1; i--) {
            variadics = cons(args[i], variadics);
        }
        args[len-1] = variadics;
    } else {
        if (len != args.length)
            invalid_arguments(wrap(l), args);
    }

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
SXI execute_(Code* code, ExecStack fiber, Environment* env) {
    SXI tos = c_void;
    opcode* ip = code->insns;
    ExecStack::ReturnFrame* cont = &fiber.frames.back();
    int fp = 0;

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
    fiber.stack.push(tos);
    ip += 1;
    NEXT;

OP(op_alloc_cont):
    fiber.frames.push({
        .code = code,
        .ip = nullptr,
        .env = env,
        .args_begin = fp,
        .args_end = fiber.stack.length,
    });
    cont = &fiber.frames.back();
    ip += 1;
    NEXT;

OP(op_alloc_stack):
    fp = fiber.stack.length;
    ip += 2;
    NEXT;

OP(op_call):
    cont->ip = ip + 1;
    goto OP(op_tailcall);

OP(op_tailcall): {
        assert(fiber.stack.length > fp);
        auto function = fiber.stack.data[fp];
        auto args = span<SXI>(fiber.stack.data + fp + 1, fiber.stack.length - fp - 1);
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
                        auto tail = fiber.stack.back();
                        fp += 1;
                        fiber.stack.length--;
                        while (tail != c_null) {
                            fiber.stack.push(car(tail));
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
                        memmove(
                            &fiber.stack[cont->args_end],
                            args.data,
                            args.length * sizeof(SXI)
                        );
                        cont->args_end += args.length;
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
        gc_run(fiber, tos);
    code = cont->code;
    ip = cont->ip;
    env = cont->env;
    fiber.stack.length = cont->args_end;
    fp = cont->args_begin;

    fiber.frames.length--;
    cont--;
    NEXT;

OP(op_exit):
    assert(fiber.stack.length == 0);
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

static ExecStack make_fiber() {
    auto exit_insns = alloc<opcode>(1);
    exit_insns[0] = op_exit;

    auto exit_fn = gc_alloc<Code>();
    *exit_fn = {};
    exit_fn->insns = exit_insns;
    exit_fn->insns_len = 1;

    ExecStack fiber = {};
    fiber.frames.push({
        .code = exit_fn, .ip = exit_fn->insns,
        .env = nullptr,
        .args_begin = 0, .args_end = 0,
    });

    return fiber;
}

SXI sxi::call(Lambda* l, int argc, SXI* argv) {
    return execute_(l->code, make_fiber(), bind_lambda(l, span(argv, argc)));
}
