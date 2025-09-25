#include "sxi.hpp"
#include "env.hpp"
#include "builtin/builtin.hpp"

using namespace sxi;

void sxi::env_define(Environment* env, const Symbol* name, SXI value) {
    env->define(name, value);
}

void sxi::env_set(Environment* env, const Symbol* name, SXI value) {
    env->set(name, value);
}

SXI sxi::env_lookup(const Environment* env, const Symbol* name) {
    // I'm too lazy to properly add const overloads for everything
    return const_cast<Environment*>(env)->lookup(name);
}

bool sxi::env_try_lookup(const Environment* env, const Symbol* name, SXI* out) {
    return const_cast<Environment*>(env)->try_lookup(name, out);
}

Environment* sxi::make_environment_rootlet() {
    auto env = make_environment();

    auto add = [&env](const builtin_lib& lib) {
        for (auto [name, func] : lib.function_1)
            env->define(make_symbol(name), wrap(func));
        for (auto [name, func] : lib.function_n)
            env->define(make_symbol(name), wrap(func));
    };

    add(builtin_lib_list);

    return env;
}

Environment* sxi::interaction_environment() {
    return make_environment(make_environment_rootlet());
}

SXI sxi::eval(SXI expr, Environment* env) {
    static auto eval_sym = make_symbol("eval");
    static auto quote = wrap(make_symbol("quote"));
    SXI eval;
    if (env->try_lookup(eval_sym, &eval))
        expr = list({eval, list({quote, expr}), wrap(env)});
    return execute(compile(expr, env));
}
