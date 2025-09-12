#pragma once

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

namespace sxi {

enum sxi_tag : char {
    SXI_TAG_const = 0,
    SXI_TAG_int,
    SXI_TAG_sym,

    SXI_TAG_pair,
    SXI_TAG_env,
    SXI_TAG_vector,
    SXI_TAG_struct_type,
    SXI_TAG_struct_instance,

    SXI_TAG_function,
    SXI_TAG_lambda,
    SXI_TAG_thunk,

    SXI_TAG_formals,
    SXI_TAG_code,
};

// The core scheme data type used throughout the interpreter
// It's small and acts like a value type so should be passed
// by value where possible.

typedef struct SXI {

    union {
        intptr_t _pad;
        sxi_tag _tag;
    };
    union {
        intptr_t _integer;
        void* _pointer;
    };

} SXI;

// Direct bitwise comparison.
// Tests pointer equality (boxed types) or value equality
// (unboxed types). Meets (and exceeds) the requirements for
// schemes 'eq?' procedure

inline bool operator==(SXI l, SXI r) {
    return l._pad == r._pad && l._integer == r._integer;
}

// Error handling (c++)
[[noreturn]] void error(const char* msg);
[[noreturn]] void error_f(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
[[noreturn]] void type_error(sxi_tag expected, SXI actual);

struct Error { static const char* what(); };

// Our own type traits, used to tell c++ how to convert to/from
// the SXI variant. tt::traits is specialised for each thing we
// can pack or unpack

namespace tt {
template <typename T> struct traits;

template <sxi_tag t> struct tagged { static constexpr sxi_tag tag = t; };
struct boxed { static constexpr bool is_boxed = true; };
struct unboxed { static constexpr bool is_unboxed = true; };

template<typename T> constexpr sxi_tag tag = traits<T>::tag;
}

template<typename T>
requires (tt::traits<T>::is_unboxed)
inline SXI
wrap(T t) {
    return { ._tag = tt::tag<T>, ._integer=static_cast<intptr_t>(t) };
}

template<typename T>
requires (tt::traits<T>::is_boxed)
inline SXI
wrap(T* t) {
    return { ._tag = tt::tag<T>, ._pointer=static_cast<void*>(t) };
}

// inline SXI wrap(SXI s) { return s; }

template <typename T>
requires(tt::traits<T>::is_unboxed)
inline T
as(SXI s) {
    if (s._tag != tt::tag<T>)
        type_error(tt::tag<T>, s);
    return static_cast<T>(s._integer);
}

template <typename T>
requires(tt::traits<T>::is_boxed)
inline T*
as(SXI s) {
    if (s._tag != tt::tag<T>)
        type_error(tt::tag<T>, s);
    return static_cast<T*>(s._pointer);
}

/// Constants ///

enum sxi_constant {
    SXI_CONST_false = 0,
    SXI_CONST_true,
    SXI_CONST_void,
    SXI_CONST_eof,
    SXI_CONST_null, // empty list
};
template<> struct tt::traits<sxi_constant> : tt::tagged<SXI_TAG_const>, tt::unboxed {};

constexpr SXI c_null  = { ._tag = SXI_TAG_const, ._integer = SXI_CONST_null  };
constexpr SXI c_true  = { ._tag = SXI_TAG_const, ._integer = SXI_CONST_true  };
constexpr SXI c_false = { ._tag = SXI_TAG_const, ._integer = SXI_CONST_false };
constexpr SXI c_eof   = { ._tag = SXI_TAG_const, ._integer = SXI_CONST_eof   };
constexpr SXI c_void  = { ._tag = SXI_TAG_const, ._integer = SXI_CONST_void  };

/// Booleans ///
// Booleans are not treated as a full distinct data type, instead
// they're bundled with the other constants. There's no wrap()/as()
// methods, instead here's some accessors

inline SXI wrap_bool(bool b) {
    return b ? c_true : c_false;
}

inline bool is_truthy(SXI sxi) {
    return sxi != c_false;
}

/// Fixnums ///
// The default numeric type is a full pointer sized integer.

typedef intptr_t sxi_int;
template<> struct tt::traits<sxi_int> : tt::tagged<SXI_TAG_int>, tt::unboxed {};


/// Pairs ///
// Cons pairs - the fundamental data structure.
// These MUST be heap allocated (like all compound data types here)
// otherwise we break the garbage collector

struct Pair {
    SXI first;
    SXI second;
};
template <> struct tt::traits<Pair> : tt::tagged<SXI_TAG_pair>, tt::boxed {};

Pair* alloc_pair();

inline Pair* make_pair(SXI first, SXI second) {
    auto p = alloc_pair();
    *p = { .first = first, .second = second };
    return p;
}

inline SXI cons(SXI first, SXI second) {
    return wrap(make_pair(first, second));
}

/// Symbols (interned) ///

struct Symbol;
template <> struct tt::traits<Symbol> : tt::tagged<SXI_TAG_sym>, tt::boxed {};

static constexpr int MAX_SYMBOL_LENGTH = 100;

Symbol* make_symbol(const char* str, int len);
const char* symbol_name(const Symbol* sym);
size_t symbol_table_size();
Symbol* gensym();
// uint32_t symbol_hash(const Symbol* sym);

/// Environments ///

struct Environment;
template <> struct tt::traits<Environment> : tt::tagged<SXI_TAG_env>, tt::boxed {};

Environment* make_environment(Environment* parent);
Environment* make_environment_toplevel();
void env_define(Environment*, const Symbol* name, SXI value);
void env_set(Environment*, const Symbol* name, SXI value);
SXI env_lookup(Environment*, const Symbol* name);

/// Read ///

SXI read(FILE*);

/// Print ///

void print(FILE*, SXI value);
const char* get_constant_name(sxi_constant c);
const char* get_tag_name(sxi_tag t);

} // sxi
