#include "sxi.hpp"
#include <acutest.h>
#include "memstream.hpp"

static void read_compare(const char* input, sxi::SXI expected) {
    auto value = sxi::read(InStream(input).f);
    TEST_CHECK(value == expected);
}

static void read_print_compare(const char* input, const char* expected) {
    InStream in{input};
    OutStream out;

    auto value = sxi::read(in.f);
    sxi::print(out.f, value);
    TEST_CHECK(out.str() == expected);
    TEST_MSG("expected: %s", expected);
    TEST_MSG("actual:   %s", out.ptr);
}

using namespace sxi;

void test_read_int() {
    read_compare("123", wrap(123z));
    read_compare("0", wrap(0z));

    InStream in{"1 23 456 7890"};
    TEST_CHECK(read(in.f) == wrap(1z));
    TEST_CHECK(read(in.f) == wrap(23z));
    TEST_CHECK(read(in.f) == wrap(456z));
    TEST_CHECK(read(in.f) == wrap(7890z));
}

void test_read_constants() {
    read_compare("", c_eof);
    read_compare("#t", c_true);
    read_compare("#f", c_false);
    read_compare("()", c_null);
}

void test_read_list() {
    read_print_compare("(1  2  3)", "(1 2 3)");
    read_print_compare("(1 . 2)", "(1 . 2)");
    read_print_compare("(1 . (2 . (3 . ())))", "(1 2 3)");
}

void test_read_symbols() {
    read_print_compare("abc", "abc");
    read_print_compare("(a b c)", "(a b c)");
}

void test_read_quote() {
    read_print_compare("'1", "(quote 1)");
    read_print_compare("'(1 2)", "(quote (1 2))");
    // read_print_compare("'", "(quote #eof)"); <-- desired?
}

TEST_LIST = {
    { "test_read_int", test_read_int },
    { "test_read_constants", test_read_constants },
    { "test_read_list", test_read_list },
    { "test_read_symbols", test_read_symbols },
    { "test_read_quote", test_read_quote },
    { NULL, NULL },
};
