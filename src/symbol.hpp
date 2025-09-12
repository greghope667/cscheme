#pragma once

#include "sxi.hpp"

namespace sxi {
namespace sym_impl {

static constexpr uintptr_t GENSYM_BASE = (1zu << 63);

struct InternedSymbol {
    uint32_t hash;
    int length;
    char data[];
};

} // impl

inline uint32_t symbol_hash(const sxi::Symbol* sym) {
    auto n = (uintptr_t)sym;
    if (n < sym_impl::GENSYM_BASE)
        return reinterpret_cast<const sym_impl::InternedSymbol*>(sym)->hash;
    else
        return static_cast<uint32_t>(n);
}

} // sxi
