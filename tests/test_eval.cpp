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

static void test_if() {
    eval_and_compare("(if #t 1 2)", "1");
    eval_and_compare("(if #f 1 2)", "2");
    eval_and_compare("(if #t 1)", "1");
    eval_and_compare("(if #f abc)", "#void");
    eval_and_compare("(if 0 'a)", "a");
    eval_and_compare("(if 'a 'b)", "b");
    eval_and_compare("(if (if #f #t #f) 2 3)", "3");
}

static void test_begin() {
    eval_and_compare("(begin)", "#void");
    eval_and_compare("(begin 1)", "1");
    eval_and_compare("(begin (begin 1) 2)", "2");
    eval_and_compare("(begin 1 (begin 2 3) (begin 4 5))", "5");
}

static void test_define() {
    eval_and_compare("(begin (define abc 1) abc)", "1");
}

static void test_set() {
    eval_and_compare(R"(
        (begin
            (define a 1)
            (set! a 2)
            a)
        )", "2");
}

TEST_LIST = {
    { "test_constant", test_constant },
    { "test_quote", test_quote },
    { "test_undefined", test_undefined },
    { "test_lookup", test_lookup },
    { "test_if", test_if },
    { "test_begin", test_begin },
    { "test_define", test_define },
    { "test_set!", test_set },
    { NULL, NULL},
};
