#pragma once

#include "sxi.hpp"
#include "../tl.hpp"
#include "../common.hpp" // IWYU pragma: export

struct function_1_def {
    const char* name;
    sxi::Function_1 function;
};

struct function_n_def {
    const char* name;
    sxi::Function_n function;
};

struct builtin_lib {
    sxi::span<const function_1_def> function_1;
    sxi::span<const function_n_def> function_n;

    template <size_t A, size_t B>
    consteval builtin_lib(const function_1_def (&a)[A], const function_n_def(&b)[B])
    : function_1(&a[0], A), function_n(&b[0], B) {}
};

#define SXI_CHECK_ARITY(min, max) \
    do { \
        if (argc < min || argc > max) \
            sxi::invalid_arguments(__FUNCTION__, sxi::span(argv, argc)); \
    } while (0)

namespace sxi {
extern const builtin_lib builtin_lib_list;
extern const builtin_lib builtin_lib_bool;
extern const builtin_lib builtin_lib_struct;
extern const builtin_lib builtin_lib_number;
extern const builtin_lib builtin_lib_misc;
extern const builtin_lib builtin_lib_vector;

static inline const builtin_lib builtin_libraries[] = {
    builtin_lib_list,
    builtin_lib_bool,
    builtin_lib_struct,
    builtin_lib_number,
    builtin_lib_misc,
    builtin_lib_vector,
};

Environment* run_init_scm_code();
}
