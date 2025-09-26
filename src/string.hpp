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

struct String {
    vector<char> _storage;
    void dealloc() { _storage.dealloc(); };

    auto data() { return _storage.data; };
    auto length() { return _storage.data ? _storage.length - 1 : 0; };

    auto begin(this auto&& self) { return self.data(); }
    auto end(this auto&& self) { return self.data() + self.length(); }

    void push(character ch) {
        if (_storage.data) {
            _storage[_storage.length - 1] = ch;
        } else {
            _storage.push(ch);
        }
        _storage.push(0);
    }

    char& operator[](int i) { bounds_check(i, length()); return data()[i]; }
    sxi::span<char> span() { return { data(), length() }; }
};

}
