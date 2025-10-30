#include "code.hpp"
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
    add(builtin_lib_bool);
    add(builtin_lib_struct);
    add(builtin_lib_number);
    add(builtin_lib_misc);
    add(builtin_lib_vector);

    env->define(make_symbol("apply"), wrap(SXI_FUNC_apply));
    env->define(make_symbol("current-env"), wrap(SXI_FUNC_current_env));
    env->define(make_symbol("values"), wrap(SXI_FUNC_values));

    return env;
}

Environment* sxi::interaction_environment() {
    static Environment* env;
    if (not env) env = run_init_scm_code();
    return make_environment(env);
}

static auto eval_sym = make_symbol("eval");

SXI sxi::eval(SXI expr, Environment* env) {
    SXI eval;
    if (env->try_lookup(eval_sym, &eval))
        expr = list({eval, quote(expr), wrap(env)});
    return execute(compile(expr, env));
}
