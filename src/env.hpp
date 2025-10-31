#pragma once

#include "sxi.hpp"
#include "tl.hpp"
#include "symbol.hpp"

namespace sxi {

struct Environment {

    Environment* parent;
    insert_only_map<const Symbol*, SXI, sxi::symbol_hash> table;

    void define(const Symbol* name, SXI value) {
        auto hash = symbol_hash(name);
        auto [found, loc] = table.lookup(name, hash);
        if (found)
            table.items[loc].value = value;
        else
            table.append(name, value, loc);
    }

    void set(const Symbol* name, SXI value) {
        auto hash = symbol_hash(name);
        for (auto* env = this; env; env = env->parent) {
            auto [found, loc] = env->table.lookup(name, hash);
            if (found) {
                env->table.items[loc].value = value;
                return;
            }
        }
        error_f("variable %s not defined", sxi::symbol_name(name));
    }

    SXI lookup(const Symbol* name) {
        auto hash = symbol_hash(name);
        for (auto* env = this; env; env = env->parent) {
            auto [found, loc] = env->table.lookup(name, hash);
            if (found)
                return env->table.items[loc].value;
        }
        error_f("variable %s not defined", sxi::symbol_name(name));
    }

    bool try_lookup(const Symbol* name, SXI* value_out) {
        auto hash = symbol_hash(name);
        for (auto* env = this; env; env = env->parent) {
            auto [found, loc] = env->table.lookup(name, hash);
            if (found) {
                *value_out = env->table.items[loc].value;
                return true;
            }
        }
        return false;
    }

    SXI_POOL_ALLOC
};

const char* get_function_name(uintptr_t address);

} // sxi
