#pragma once

#include "sxi.hpp"
#include "tl.hpp"

namespace sxi {

[[noreturn]] void error(SXI obj, const char* msg);
[[noreturn]] void invalid_arguments(SXI function, span<SXI> args);
[[noreturn]] void invalid_arguments(const char* funcname, span<SXI> args);

constexpr struct { const char* name; sxi::character ch; } character_names[] = {
    { "alarm",     '\a' },
    { "backspace", '\b' },
    { "delete",    0x7f },
    { "escape",    '\e' },
    { "newline",   '\n' },
    { "null",      '\0' },
    { "return",    '\r' },
    { "space",      ' ' },
    { "tab",       '\t' },
};

constexpr struct { char escape; sxi::character ch; } string_escapes[] = {
    { 'a' , '\a' },
    { 'b' , '\b' },
    { 'e' , '\e' },
    { 't' , '\t' },
    { 's' ,  ' ' },
    { 'n' , '\n' },
    { 'r' , '\r' },
    { '"' ,  '"' },
    { '\\', '\\' },
    { '|' ,  '|' },
    { '0' , '\0' },
};

struct String : string { SXI_POOL_ALLOC };

struct Vector : vector<SXI> { SXI_POOL_ALLOC };

struct StructType {
    Symbol* name;
    vector<Symbol*> field_names;
    SXI_POOL_ALLOC
};
template <> struct tt::traits<StructType> : tt::tagged<SXI_TAG_struct_type>, tt::boxed {};

struct StructInstance {
    StructType* type;
    vector<SXI> fields;
    SXI_POOL_ALLOC
};
template <> struct tt::traits<StructInstance> : tt::tagged<SXI_TAG_struct_instance>, tt::boxed {};

}
