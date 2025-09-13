#include "sxi.hpp"
#include <stdlib.h>
#include "env.hpp"

sxi::Pair* sxi::alloc_pair() {
    return new sxi::Pair;
}

sxi::Environment* sxi::make_environment(Environment* parent) {
    auto env = new sxi::Environment{};
    env->parent = parent;
    return env;
}
