#include "sxi.hpp"
#include <acutest.h>
#include <stdio.h>

using namespace sxi;

static void test_define() {
    auto env = make_environment(nullptr);
    auto sym = make_symbol("1");
    auto val = wrap(1z);
    env_define(env, sym, val);
    TEST_CHECK(env_lookup(env, sym) == val);
}

static void test_define_many() {
    auto env = make_environment(nullptr);
    char buf[10];

    for (int i=0; i<10; i++) {
        sprintf(buf, "%i", i);
        env_define(env, make_symbol(buf), wrap(1z));
    }

    for (int i=0; i<10; i++) {
        sprintf(buf, "%i", i);
        TEST_CHECK(env_lookup(env, make_symbol(buf)) == wrap(1z));
    }
}

static void test_define_gensym() {
    auto env = make_environment(nullptr);
    Symbol* syms[20];
    for (int i=0; i<20; i++) {
        syms[i] = gensym();
        env_define(env, syms[i], wrap(1z));
    }

    for (int i=0; i<20; i++) {
        TEST_CHECK(env_lookup(env, syms[i]) == wrap(1z));
    }
}

static void test_undefined() {
    auto env = make_environment(nullptr);
    TEST_EXCEPTION(
        env_lookup(env, gensym());
    , Error);
    TEST_EXCEPTION(
        env_set(env, gensym(), wrap(1z));
    , Error);
}

static void test_nested() {
    auto parent = make_environment(nullptr);
    auto child = make_environment(parent);
    auto sym = gensym();
    env_define(parent, sym, wrap(1z));
    TEST_CHECK(env_lookup(child, sym) == wrap(1z));
}

TEST_LIST = {
    { "test_define", test_define },
    { "test_define_may", test_define_many },
    { "test_define_gensym", test_define_gensym },
    { "test_undefined", test_undefined },
    { "test_nested", test_nested },
    { NULL, NULL },
};
