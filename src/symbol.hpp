#pragma once

#include "sxi.hpp"

namespace sxi {

struct Symbol {};

struct InternedSymbol : Symbol {
    uint32_t hash;
    int length;
    char data[];
};

static constexpr uintptr_t GENSYM_BASE = (1zu << 63);

inline uint32_t symbol_hash(const sxi::Symbol* sym) {
    auto n = (uintptr_t)sym;
    if (n < GENSYM_BASE)
        return static_cast<const InternedSymbol*>(sym)->hash;
    else
        return static_cast<uint32_t>(n);
}

} // sxi
