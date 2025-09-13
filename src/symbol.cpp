#include "symbol.hpp"
#include "sxi.hpp"
#include "utils.hpp"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

using namespace sxi;

Symbol* sxi::make_symbol(const char* str, int len) {
    if ((unsigned)len > MAX_SYMBOL_LENGTH)
        error("symbol too long");

    uint32_t hash = 0x811c9dc5;
    for (int i=0; i<len; i++) {
        hash ^= (unsigned char)str[i];
        hash *= 0x01000193;
    }

    static constinit vector<InternedSymbol*> intern_table = {};

    for (auto* intern : intern_table) {
        if (intern->hash == hash && intern->length == len) {
            if (memcmp(intern->data, str, len) == 0)
                return static_cast<Symbol*>(intern);
        }
    }

    auto* sym = (InternedSymbol*)malloc(sizeof(InternedSymbol) + len + 1);
    memcpy(&sym->data, str, len);
    sym->data[len] = 0;
    sym->length = len;
    sym->hash = hash;

    intern_table.push(sym);

    return static_cast<Symbol*>(sym);
}

Symbol* sxi::make_symbol(const char* str) {
    return make_symbol(str, strlen(str));
}

const char* sxi::symbol_name(const Symbol* sym) {
    auto n = (uintptr_t)sym;
    if (n < GENSYM_BASE) {
        return static_cast<const InternedSymbol*>(sym)->data;
    } else {
        static char gensym_name[32] = {};
        snprintf(gensym_name, sizeof(gensym_name), "#<gensym%zx>", n - GENSYM_BASE);
        return gensym_name;
    }
}

Symbol* sxi::gensym() {
    static constinit auto next = GENSYM_BASE + 1;
    return (Symbol*)next++;
}

size_t symbol_table_size();
