#include "match.hpp"
#include "sxi.hpp"
#include "code.hpp"
#include "alloc.hpp"
#include "env.hpp"
#include "common.hpp"

using namespace sxi;

static Environment*
bind_lambda(Lambda* l, span<SXI> args) {
    int len = l->arguments->names.length;

    auto env = new Environment{ .parent = l->capture, .table = {} };
    env->table.items.reserve(len);
    env->table.items.length = len;

    for (int i=0; i<len; i++) {
        env->table.items[i] = {
            .key = l->arguments->names[i],
            .value = c_false,
        };
    }

    if (l->arguments->is_variadic) {
        if (len - 1 > args.length)
            invalid_arguments(wrap(l), args);

        for (int i=0; i<len-1; i++)
            env->table.items[i].value = args[i];

        auto variadics = c_null;
        for (int i = args.length-1; i >= len-1; i--) {
            variadics = cons(args[i], variadics);
        }

        env->table.items[len-1].value = variadics;
    } else {
        if (len != args.length)
            invalid_arguments(wrap(l), args);
        for (int i=0; i<len; i++)
            env->table.items[i].value = args[i];
    }

    return env;
}

using Stack = vector<SXI>;

#if __has_attribute(preserve_none)
#define PRESERVE_NONE __attribute__((preserve_none))
#else
// #warning preserve_none not supported
#define PRESERVE_NONE
#endif

#define MUSTTAIL __attribute((musttail))

#define ARGS (Fiber* fiber, opcode* ip, Code* code, Stack* stack, Environment* locals)
#define NEXT(l) MUSTTAIL return dispatch_table[ip[l]](fiber, ip+l, code, stack, locals)
#define JUMP(op) MUSTTAIL return (ex_ ## op)(fiber, ip, code, stack, locals)

using opcode_fn = PRESERVE_NONE SXI ARGS;

#define X(op) static opcode_fn ex_ ## op;
OPCODES
#undef X

static constexpr opcode_fn* dispatch_table[] = {
    #define X(op) ex_ ## op,
    OPCODES
    #undef X
};

#define EX(op) static PRESERVE_NONE SXI ex_ ## op ARGS

EX(op_exit) {
    SXI_ASSERT(stack->length == 0);
    SXI_ASSERT(fiber->frames.length == 0);
    (void)ip;
    (void)code;
    (void)locals;
    return fiber->tos;
}

EX(op_unlambda) {
    locals = locals->parent;
    NEXT(1);
}

EX(op_literal) {
    fiber->tos = code->literals[ip[1]];
    NEXT(2);
}

EX(op_lambda) {
    auto pl = code->lambdas[ip[1]];
    auto lambda = new Lambda{ pl.code, pl.arguments, locals };
    fiber->tos = wrap(lambda);
    NEXT(2);
}

EX(op_push) {
    stack->push(fiber->tos);
    NEXT(1);
}

EX(op_lookup) {
    fiber->tos = locals->lookup(code->symbols[ip[1]]);
    NEXT(2);
}

EX(op_define) {
    locals->define(code->symbols[ip[1]], fiber->tos);
    NEXT(2);
}

EX(op_set) {
    locals->set(code->symbols[ip[1]], fiber->tos);
    NEXT(2);
}

EX(op_branch) {
    auto dist = ip[1];
    NEXT(dist);
}

EX(op_branch0) {
    auto dist = is_truthy(fiber->tos) ? 2 : ip[1];
    NEXT(dist);
}

EX(op_alloc_cont) {
    fiber->frames.push({
        .code = code,
        .ip = nullptr,
        .env = locals,
        .args_begin = fiber->cont->args_end,
        .args_end = stack->length,
    });
    fiber->cont = &fiber->frames.back();
    NEXT(1);
}

EX(op_alloc_stack) {
    stack->length = fiber->cont->args_end;
    NEXT(2);
}

EX(op_ret) {
    if (gc_allocations > 4096)
        fiber->gc_run();
    auto cont = fiber->cont;
    code = cont->code;
    ip = cont->ip;
    locals = cont->env;
    stack->length = cont->args_end;
    fiber->frames.length--;
    fiber->cont--;
    NEXT(0);
}

EX(op_call) {
    fiber->cont->ip = ip + 1;
    JUMP(op_tailcall);
}

EX(op_tailcall) {
    int fp = fiber->cont->args_end;
    SXI_ASSERT(stack->length > fp);
    auto function = stack->data[fp];
    auto args = span<SXI>(stack->data + fp + 1, stack->length - fp - 1);
    match(function) {
        case_val(Function_n, f) {
            fiber->tos = f(args.length, args.data);
            JUMP(op_ret);
        }
        case_val(Function_1, f) {
            if (args.length != 1)
                invalid_arguments(function, args);
            fiber->tos = f(args[0]);
            JUMP(op_ret);
        }
        case_val(Function_s, f) {
            switch (f) {
                case SXI_FUNC_apply: {
                    if (args.length < 2)
                        invalid_arguments(function, args);
                    auto tail = stack->back();
                    memmove(stack->data + fp, stack->data + fp + 1, (args.length-1)*sizeof(SXI));
                    stack->length -= 2;
                    while (tail != c_null) {
                        stack->push(car(tail));
                        tail = cdr(tail);
                    }
                    JUMP(op_tailcall);
                }
                case SXI_FUNC_current_env: {
                    if (args.length > 0)
                        invalid_arguments(function, args);
                    fiber->tos = wrap(locals);
                    JUMP(op_ret);
                }
                case SXI_FUNC_values: {
                    if (fiber->cont->ip[0] != op_push) {
                        if (args.length != 1)
                            error("cannot return values to single-valued continuation");
                        fiber->tos = args[0];
                        JUMP(op_ret);
                    }
                    fiber->cont->ip++;
                    memmove(
                        &stack->data[fiber->cont->args_end],
                        args.data,
                        args.length * sizeof(SXI)
                    );
                    fiber->cont->args_end += args.length;
                    JUMP(op_ret);
                }
                default:
                    SXI_UNREACHABLE;
            }
        }
        case_lambda(l) {
            locals = bind_lambda(l, args);
            code = l->code;
            ip = code->insns;
            NEXT(0);
        }
        default:
            error(function, "object is not callable");
    }
}

static Fiber make_fiber(Environment* globals) {
    auto exit_insns = alloc<opcode>(1);
    exit_insns[0] = op_exit;

    auto exit_fn = new Code{};
    exit_fn->insns = exit_insns;
    exit_fn->insns_len = 1;

    Fiber fiber = {};
    fiber.frames.push({
        .code = exit_fn, .ip = exit_fn->insns,
        .env = nullptr,
        .args_begin = 0, .args_end = 0,
    });

    fiber.cont = &fiber.frames.back();
    fiber.globals = globals;

    return fiber;
}

static void print_stacktrace(Fiber& fiber) {
    int depth = 0;
    for (auto& frame : fiber.frames) {
        printf("Frame %i: [%i:%i]\n\t", depth++, frame.args_begin, frame.args_end);
        for (int i = frame.args_begin; i<frame.args_end; i++) {
            print(stdout, fiber.stack[i]);
            putchar(' ');
        }
        putchar('\n');
    }
    printf("Frame %i: [%i:%i]\n\t", depth, fiber.cont->args_end, fiber.stack.length);
    for (int i = fiber.cont->args_end; i<fiber.stack.length; i++) {
        print(stdout, fiber.stack[i]);
        putchar(' ');
    }
    putchar('\n');
}

SXI sxi::call(Lambda* l, int argc, SXI* argv) {
    auto locals = bind_lambda(l, span(argv, argc));
    auto fiber = make_fiber(l->capture);
    auto ip = l->code->insns;
    SXI v;
    SXI_TRY {
        v = dispatch_table[*ip](&fiber, ip, l->code, &fiber.stack, locals);
        fiber.stack.dealloc();
        fiber.frames.dealloc();
    } SXI_CATCH(_) {
        print_stacktrace(fiber);
        fiber.stack.dealloc();
        fiber.frames.dealloc();
        SXI_THROW;
    }
    return v;
}
