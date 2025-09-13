#include "sxi.hpp"
#include "env.hpp"

using namespace sxi;

void sxi::env_define(Environment* env, const Symbol* name, SXI value) {
    env->define(name, value);
}

void sxi::env_set(Environment* env, const Symbol* name, SXI value) {
    env->set(name, value);
}

SXI sxi::env_lookup(Environment* env, const Symbol* name) {
    return env->lookup(name);
}
