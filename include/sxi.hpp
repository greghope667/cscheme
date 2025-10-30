#pragma once

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

namespace sxi {

enum sxi_tag : char {
    SXI_TAG_const = 0,
    SXI_TAG_int,
    SXI_TAG_sym,
    SXI_TAG_char,

    SXI_TAG_pair,
    SXI_TAG_env,
    SXI_TAG_vector,
    SXI_TAG_struct_type,
    SXI_TAG_struct_instance,

    SXI_TAG_string,

    SXI_TAG_function_n,
    SXI_TAG_function_1,
    SXI_TAG_function_s,
    SXI_TAG_lambda,
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

static constexpr inline bool operator==(SXI l, SXI r) {
    return l._pad == r._pad && l._integer == r._integer;
}

bool equal(SXI l, SXI r);

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
struct fptr { static constexpr bool is_fptr = true; };

template <typename T> concept Unboxed = traits<T>::is_unboxed;
template <typename T> concept Boxed = traits<T>::is_boxed;
template <typename T> concept Fptr = traits<T>::is_fptr;

template <typename T> constexpr sxi_tag tag = traits<T>::tag;
}

template <tt::Unboxed T>
static constexpr inline SXI
wrap(T t) {
    return { ._pad = tt::tag<T>, ._integer=static_cast<intptr_t>(t) };
}

template <tt::Boxed T>
static constexpr inline SXI
wrap(T* t) {
    return { ._pad = tt::tag<T>, ._pointer=static_cast<void*>(t) };
}

template <tt::Fptr T>
static constexpr inline SXI
wrap(T t) {
    return { ._pad = tt::tag<T>, ._pointer=reinterpret_cast<void*>(t) };
}

static constexpr inline SXI
wrap(SXI v) { return v; }

template <tt::Unboxed T>
static constexpr inline T
as(SXI s) {
    if (s._tag != tt::tag<T>)
        type_error(tt::tag<T>, s);
    return static_cast<T>(s._integer);
}

template <tt::Boxed T>
static constexpr inline T*
as(SXI s) {
    if (s._tag != tt::tag<T>)
        type_error(tt::tag<T>, s);
    return static_cast<T*>(s._pointer);
}

template <tt::Fptr T>
static constexpr inline T
as(SXI s) {
    if (s._tag != tt::tag<T>)
        type_error(tt::tag<T>, s);
    return reinterpret_cast<T>(s._pointer);
}

template <typename T>
static constexpr inline bool
instance(SXI s) {
    return s._tag == tt::tag<T>;
}

/// Constants ///

enum sxi_constant {
    SXI_CONST_false = 0,
    SXI_CONST_true,
    SXI_CONST_void,
    SXI_CONST_eof,
    SXI_CONST_null, // empty list
};
template <> struct tt::traits<sxi_constant> : tt::tagged<SXI_TAG_const>, tt::unboxed {};

// constexpr SXI c_null  = { ._tag = SXI_TAG_const, ._integer = SXI_CONST_null  };
constexpr SXI c_null  = wrap(SXI_CONST_null);
constexpr SXI c_true  = wrap(SXI_CONST_true);
constexpr SXI c_false = wrap(SXI_CONST_false);
constexpr SXI c_eof   = wrap(SXI_CONST_eof);
constexpr SXI c_void  = wrap(SXI_CONST_void);

/// Booleans ///
// Booleans are not treated as a full distinct data type, instead
// they're bundled with the other constants. There's no wrap()/as()
// methods, instead here's some accessors

static constexpr inline SXI wrap_bool(bool b) {
    return b ? c_true : c_false;
}

static constexpr inline bool is_truthy(SXI sxi) {
    return sxi != c_false;
}

/// Fixnums ///
// The default numeric type is a full pointer sized integer.

typedef intptr_t sxi_int;
template <> struct tt::traits<sxi_int> : tt::tagged<SXI_TAG_int>, tt::unboxed {};

/// Characters (Unicode? Who uses that?) ///

typedef unsigned char character;
template <> struct tt::traits<character> : tt::tagged<SXI_TAG_char>, tt::unboxed {};

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

static inline Pair* make_pair(SXI first, SXI second) {
    auto p = alloc_pair();
    *p = { .first = first, .second = second };
    return p;
}

static inline SXI cons(SXI first, SXI second) {
    return wrap(make_pair(first, second));
}

template <size_t N>
static inline SXI list(const SXI (&ls)[N]) {
    SXI head = c_null;
    for (auto i = N; i --> 0;)
        head = cons(ls[i], head);
    return head;
}

static inline SXI car(SXI p) { return as<Pair>(p)->first; }
static inline SXI cdr(SXI p) { return as<Pair>(p)->second; }

/// Symbols (interned) ///

struct Symbol;
template <> struct tt::traits<Symbol> : tt::tagged<SXI_TAG_sym>, tt::boxed {};

static constexpr int MAX_SYMBOL_LENGTH = 100;

Symbol* make_symbol(const char* str, int len);
Symbol* make_symbol(const char* str);
const char* symbol_name(const Symbol* sym);
size_t symbol_table_size();
Symbol* gensym();

namespace symbols {
    extern Symbol* const quote;
}

/// Strings ///

struct String;
template <> struct tt::traits<String> : tt::tagged<SXI_TAG_string>, tt::boxed {};

String* make_string();
String* make_string(const char* str);
String* make_string(const char* str, int len);
char* string_data(String*, int* len = nullptr);

/// Vectors ///

struct Vector;
template <> struct tt::traits<Vector> : tt::tagged<SXI_TAG_vector>, tt::boxed {};

Vector* make_vector(int len = 0);
SXI* vector_data(Vector*, int* len = nullptr);
SXI* vector_ref(Vector*, int index);
void vector_push(Vector*, SXI value);

/// Environments ///

struct Environment;
template <> struct tt::traits<Environment> : tt::tagged<SXI_TAG_env>, tt::boxed {};

Environment* make_environment(Environment* parent = nullptr);
Environment* make_environment_rootlet();
Environment* interaction_environment();
void env_define(Environment*, const Symbol* name, SXI value);
void env_set(Environment*, const Symbol* name, SXI value);
SXI env_lookup(const Environment*, const Symbol* name);
bool env_try_lookup(const Environment*, const Symbol* name, SXI* value_out);

/// Lambdas ///

struct Lambda;
template <> struct tt::traits<Lambda> : tt::tagged<SXI_TAG_lambda>, tt::boxed {};

SXI call(Lambda*, int argc, SXI* argv);
static inline SXI execute(Lambda* l) { return call(l, 0, 0); }
Lambda* compile(SXI expr, Environment* env);
void disassemble(FILE*, const Lambda*);

template <size_t N>
static SXI call(Lambda* l, SXI (&&args)[N]) {
    return call(l, N, args);
}

/// C-Functions ///

using Function_n = SXI (*)(int, SXI*);
template <> struct tt::traits<Function_n> : tt::tagged<SXI_TAG_function_n>, tt::fptr {};

using Function_1 = SXI (*)(SXI);
template <> struct tt::traits<Function_1> : tt::tagged<SXI_TAG_function_1>, tt::fptr {};

/// Read ///

SXI read(FILE*);

/// Eval ///

SXI eval(SXI expr, Environment* env);
static inline SXI quote(SXI value) {
    return cons(wrap(symbols::quote), cons(value, c_null));
}

/// Print ///

void print(FILE*, SXI value);
const char* get_constant_name(sxi_constant c);
const char* get_tag_name(sxi_tag t);

/// GC ///

void gc_protect(SXI value);
void gc_unprotect(SXI value);

} // sxi

#ifndef SXI_USE_SETJMP
#define SXI_TRY try
#define SXI_CATCH(e) catch (sxi::Error e)
#define SXI_THROW throw sxi::Error{}
#else
#include <setjmp.h>
#include <assert.h>
namespace sxi {
    struct jmp_buf_chain {
        static jmp_buf_chain* top;

        jmp_buf_chain* next;
        jmp_buf buf;

        jmp_buf_chain() : next(top) { top = this; }
        jmp_buf_chain(const jmp_buf_chain&) = delete;
        jmp_buf_chain(jmp_buf_chain&&) = delete;
        void pop() { top = next; }

        static void dothrow() __attribute__((noreturn)) {
            assert(top);
            _longjmp(top->buf, 1);
        }
    };
}
#define SXI_TRY \
    if (sxi::jmp_buf_chain _try_{}; not _setjmp(_try_.buf)) {

#define SXI_CATCH(e) \
        _try_.pop(); \
    } else if (auto e = (_try_.pop(), sxi::Error{}); true) \

#define SXI_THROW sxi::jmp_buf_chain::dothrow()
#endif
