#include "sxi.hpp"

#include <stdio.h>
#include <stdlib.h>

static auto section = wrap(sxi::make_symbol("section"));
static auto cases = wrap(sxi::make_symbol("cases"));
static auto skip = wrap(sxi::make_symbol("skip"));

static auto unspecified = wrap(sxi::make_symbol("unspecified"));
static auto eval_to = wrap(sxi::make_symbol("==>"));
static auto eval_pred = wrap(sxi::make_symbol("==>?"));

static bool run_test_case(sxi::String*, sxi::SXI case_expr, sxi::Environment* env) {
    auto expr = sxi::car(case_expr);
    auto type = sxi::car(sxi::cdr(case_expr));
    auto expected = sxi::cdr(sxi::cdr(case_expr));

    static auto list = wrap(sxi::make_symbol("list"));
    static auto quote = wrap(sxi::make_symbol("quote"));
    static auto apply = wrap(sxi::make_symbol("apply"));

    auto eval_env = sxi::make_environment(env);
    auto result = sxi::c_false;

    SXI_TRY {
        result = sxi::eval(sxi::list({list, expr}), eval_env);
    } SXI_CATCH(e) {
        printf("Execution error in test: %s", e.what());
        printf("\nTest expression: ");
        sxi::print(stdout, expr);
        putchar('\n');
        return false;
    }

    if (type == unspecified)
        // Result not important, as long as we don't throw
        return true;

    if (type == eval_to && sxi::equal(result, expected))
        return true;

    if (type == eval_pred) {
        auto r = sxi::eval(
            sxi::list({
                apply,
                sxi::car(expected),
                sxi::list({quote, result})
            }),
            eval_env);
        if (sxi::is_truthy(r)) return true;
    }

    printf("Test case evaluated to bad result:\n");
    sxi::print(stdout, expr);
    printf("\n\tExpected:  ");
    sxi::print(stdout, expected);
    printf("\n\tActual:    ");
    sxi::print(stdout, result);
    putchar('\n');
    return false;
}

const char test_cases[] = {
    #embed "r7rs-cases.scm"
};

int main() {
    auto f = fmemopen((void*)test_cases, sizeof(test_cases), "rb");
    if (not f) {
        printf("Failed to open test case file\n");
        return EXIT_FAILURE;
    }

    auto env = sxi::interaction_environment();
    sxi::gc_protect(wrap(env));

    int success_count = 0;
    int failure_count = 0;
    int skip_count = 0;

    auto section_name = sxi::make_string("[No section]");

    SXI_TRY {
        for (;;) {
            auto r = sxi::read(f);
            if (r == sxi::c_eof)
                break;

            auto [statement, body] = *sxi::as<sxi::Pair>(r);
            if (statement == section) {
                section_name = sxi::as<sxi::String>(sxi::car(body));
                printf("Section %s\n", sxi::string_data(section_name));
            } else if (statement == cases) {
                do {
                    auto success = run_test_case(section_name, sxi::car(body), env);
                    success ? success_count++ : failure_count++;
                    body = sxi::cdr(body);
                } while (body != sxi::c_null);
            } else if (statement == skip) {
                skip_count++;
            }
        }
    } SXI_CATCH(e) {
        printf("Unexpected error while running tests (aborting)\n%s\n", e.what());
        return EXIT_FAILURE;
    }

    printf("successes %i failures %i skipped %i\n", success_count, failure_count, skip_count);

    if (failure_count > 0) return EXIT_FAILURE;
}
