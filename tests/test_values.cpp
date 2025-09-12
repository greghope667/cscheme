#include "sxi.hpp"
#include <acutest.h>

using namespace sxi;

void test_eq() {
    TEST_CHECK(c_true == c_true);
    TEST_CHECK(not (c_true == c_false));
    TEST_CHECK(c_true != c_false);
}

void test_truthy() {
    TEST_CHECK(is_truthy(sxi::c_true));
    TEST_CHECK(sxi::is_truthy(c_null));
    TEST_CHECK(not is_truthy(c_false));
}

void test_wrap() {
    (void)wrap<sxi_int>(3);
    Pair pair{};
    (void)wrap<Pair>(&pair);
}

void test_throws() {
    TEST_EXCEPTION(
        as<Pair>(wrap<sxi_int>(0));
    , Error);
}

TEST_LIST = {
    { "test_eq", test_eq },
    { "test_truthy", test_truthy },
    { "test_wrap", test_wrap },
    { "text_throws", test_throws },
    { NULL, NULL },
};
