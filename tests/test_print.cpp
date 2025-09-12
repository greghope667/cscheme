#include "sxi.hpp"
#include <acutest.h>
#include <stdio.h>
#include "memstream.hpp"

void test_memstream() {
    OutStream out;
    fprintf(out.f, "test");
    TEST_CHECK(out.str() == "test");
}

using namespace sxi;

static void print_and_compare(SXI value, const char* expected) {
    OutStream out;
    print(out.f, value);
    TEST_CHECK(out.str() == expected);
    TEST_MSG("expected: %s", expected);
    TEST_MSG("actual:   %s", out.ptr);
}

void test_print_int() {
    print_and_compare(wrap<sxi_int>(0), "0");
    print_and_compare(wrap<sxi_int>(123), "123");
    print_and_compare(wrap<sxi_int>(-123456), "-123456");
}

void test_print_constants() {
    print_and_compare(c_null, "()");
    print_and_compare(c_true, "#t");
    print_and_compare(c_false, "#f");
    print_and_compare(c_eof, "#eof");
}

void test_print_list() {
    Pair a{wrap<sxi_int>(3), c_null};
    Pair b{wrap<sxi_int>(2), wrap(&a)};
    Pair c{wrap<sxi_int>(1), wrap(&b)};
    print_and_compare(wrap(&c), "(1 2 3)");
}

void test_print_improper_list() {
    Pair b{wrap(2z), wrap(3z)};
    Pair c{wrap(1z), wrap(&b)};
    print_and_compare(wrap(&c), "(1 2 . 3)");
}

TEST_LIST = {
    { "test_memstream", test_memstream },
    { "test_print_int", test_print_int },
    { "test_print_constants", test_print_constants },
    { "test_print_list", test_print_list },
    { "test_print_improper_list", test_print_improper_list },
    { NULL, NULL },
};
