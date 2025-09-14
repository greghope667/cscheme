#include "sxi.hpp"
#include "env.hpp"

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
