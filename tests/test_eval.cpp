#include "sxi.hpp"
#include <acutest.h>
#include "memstream.hpp"

static void eval_and_compare(const char* input, const char* expected) {
    auto r = sxi::read(InStream{input}.f);
    auto env = sxi::make_environment(nullptr);
    auto c = sxi::compile(r, env);
    auto e = sxi::execute(c);
    OutStream out;
    sxi::print(out.f, e);
    TEST_CHECK(out.str() == expected);
    TEST_MSG("input:    %s", input);
    TEST_MSG("expected: %s", expected);
    TEST_MSG("actual:   %s", out.ptr);
}

static void test_constant() {
    eval_and_compare("123", "123");
    eval_and_compare("#t", "#t");
    eval_and_compare("#f", "#f");
}

static void test_quote() {
    eval_and_compare("'abc", "abc");
    eval_and_compare("(quote 123)", "123");
    eval_and_compare("'(1 2 3)", "(1 2 3)");
}

static void test_undefined() {
    auto s = sxi::make_symbol("abc");
    auto env = sxi::make_environment(nullptr);
    auto c = sxi::compile(wrap(s), env);
    TEST_EXCEPTION(
        sxi::execute(c);
    , sxi::Error);
}

static void test_lookup() {
    auto s = sxi::make_symbol("abc");
    auto env = sxi::make_environment(nullptr);
    sxi::env_define(env, s, sxi::wrap(123z));
    auto c = sxi::compile(wrap(s), env);
    auto e = sxi::execute(c);
    TEST_CHECK(e == sxi::wrap(123z));
}

TEST_LIST = {
    { "test_constant", test_constant },
    { "test_quote", test_quote },
    { "test_undefined", test_undefined },
    { "test_lookup", test_lookup },
    { NULL, NULL},
};
