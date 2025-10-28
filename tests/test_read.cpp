#include "sxi.hpp"
#include <acutest.h>
#include "memstream.hpp"

static void read_compare(const char* input, sxi::SXI expected) {
    auto value = sxi::read(InStream(input).f);
    TEST_CHECK(value == expected);
    TEST_MSG("input: %s", input);
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

static void test_read_int() {
    read_compare("123", wrap(123z));
    read_compare("0", wrap(0z));

    InStream in{"1 23 456 7890"};
    TEST_CHECK(read(in.f) == wrap(1z));
    TEST_CHECK(read(in.f) == wrap(23z));
    TEST_CHECK(read(in.f) == wrap(456z));
    TEST_CHECK(read(in.f) == wrap(7890z));
}

static void test_read_constants() {
    read_compare("", c_eof);
    read_compare("#t", c_true);
    read_compare("#f", c_false);
    read_compare("()", c_null);
}

static void test_read_list() {
    read_print_compare("(1  2  3)", "(1 2 3)");
    read_print_compare("(1 . 2)", "(1 . 2)");
    read_print_compare("(1 . (2 . (3 . ())))", "(1 2 3)");
}

static void test_read_symbols() {
    read_print_compare("abc", "abc");
    read_print_compare("(a b c)", "(a b c)");
}

static void test_read_quote() {
    read_print_compare("'1", "(quote 1)");
    read_print_compare("'(1 2)", "(quote (1 2))");
    read_print_compare(
        "`(a ,b ,@c)",
        "(quasiquote (a (unquote b) (unquote-splicing c)))"
    );
    // read_print_compare("'", "(quote #eof)"); <-- desired?
}

static void test_read_comment() {
    read_print_compare("(1 2 ; some garbage \n 3)", "(1 2 3)");
    read_print_compare("; 123", "#eof");
}

static void test_read_chars() {
    struct { const char* in; sxi::character ch; } cases[] = {
        { "#\\a",         'a' },
        { "#\\A",         'A' },
        { "#\\(",         '(' },
        { "#\\ ",         ' ' },
        { "#\\x61",       'a' },
        { "#\\alarm",      7  },
        { "#\\backspace",  8  },
        { "#\\delete",   0x7f },
        { "#\\escape",   '\e' },
        { "#\\newline",  '\n' },
        { "#\\null",     '\0' },
        { "#\\return",   '\r' },
        { "#\\space",     ' ' },
        { "#\\tab",      '\t' },
    };
    for (auto [in, ch] : cases) read_compare(in, wrap(ch));
    TEST_EXCEPTION(sxi::read(InStream("#\\").f);, sxi::Error);
}

static void test_read_number_base() {
    read_compare("#b101", wrap(0b101z));
    read_compare("#o101", wrap(0101z));
    read_compare("#d101", wrap(101z));
    read_compare("#x101", wrap(0x101z));
}

static void test_read_vector() {
    read_print_compare("#(1 2 3)", "#(1 2 3)");
    read_print_compare("#(a b c)", "#(a b c)");
}

TEST_LIST = {
    { "test_read_int", test_read_int },
    { "test_read_constants", test_read_constants },
    { "test_read_list", test_read_list },
    { "test_read_symbols", test_read_symbols },
    { "test_read_quote", test_read_quote },
    { "test_read_comment", test_read_comment },
    { "test_read_chars", test_read_chars },
    { "test_read_number_base", test_read_number_base },
    { "test_read_vector", test_read_vector },
    { NULL, NULL },
};
