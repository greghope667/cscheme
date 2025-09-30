#include "sxi.hpp"
#include "tl.hpp"

namespace sxi {

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

struct String : string {};

}
