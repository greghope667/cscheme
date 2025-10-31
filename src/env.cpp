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

    for (auto& lib : builtin_libraries) {
        for (auto [name, func] : lib.function_1)
            env->define(make_symbol(name), wrap(func));
        for (auto [name, func] : lib.function_n)
            env->define(make_symbol(name), wrap(func));
    };

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
    if (env->try_lookup(eval_sym, &eval) && instance<Lambda>(eval)) {
        return call(as<Lambda>(eval), { expr, wrap(env) });
    } else {
        return execute(compile(expr, env));
    }
}

static auto function_names = []{
    insert_only_map<uintptr_t, const char*, [](auto x){ return x; }> table{};

    auto insert = [&](const char* name, uintptr_t value) {
        auto [found, loc] = table.lookup(value);
        if (not found) table.append(value, name, loc);
    };

    for (auto& lib : builtin_libraries) {
        for (auto [name, func] : lib.function_1)
            insert(name, uintptr_t(func));
        for (auto [name, func] : lib.function_n)
            insert(name, uintptr_t(func));
    }

    insert("apply", SXI_FUNC_apply);
    insert("current-env", SXI_FUNC_current_env);
    insert("values", SXI_FUNC_values);

    return table;
}();

const char* sxi::get_function_name(uintptr_t address) {
    auto [found, loc] = function_names.lookup(address);
    if (found) {
        return function_names.items[loc].value;
    } else {
        static char buf[20];
        sprintf(buf, "0x%zx", address);
        return buf;
    }
}
