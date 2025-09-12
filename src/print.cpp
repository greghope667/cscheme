#include "sxi.hpp"
#include "match.hpp"

#include <stdio.h>

using namespace sxi;

const char* sxi::get_tag_name(sxi_tag tag) {
    switch (tag) {
    case SXI_TAG_const:     return "constant";
    case SXI_TAG_int:       return "integer";
    case SXI_TAG_sym:       return "symbol";
    case SXI_TAG_pair:      return "pair";
    case SXI_TAG_env:       return "environment";
    default:                break;
    }
    static char tagname[32];
    snprintf(tagname, 32, "<tag %i>", tag);
    fprintf(stderr, "WARNING: %s has no name\n", tagname);
    return tagname;
}

const char* sxi::get_constant_name(sxi_constant c) {
    switch (c) {
    case SXI_CONST_false:   return "#f";
    case SXI_CONST_true:    return "#t";
    case SXI_CONST_void:    return "#<>";
    case SXI_CONST_eof:     return "#eof";
    case SXI_CONST_null:    return "()";
    default:                break;
    }
    static char constname[32];
    snprintf(constname, 32, "<constant 0x%x>", c);
    fprintf(stderr, "WARNING: %s has no name\n", constname);
    return constname;
}

static void print_list(FILE* f, Pair* l) {
    fputc('(', f);
    for (;;) {
        auto [head, rest] = *l;
        print(f, head);
        if (rest == c_null) {
            break;
        }
        if (rest._tag != SXI_TAG_pair) {
            fprintf(f, " . ");
            print(f, rest);
            break;
        }
        fputc(' ', f);
        l = as<Pair>(rest);
    }
    fputc(')', f);
}

void sxi::print(FILE* f, SXI value) {
    match(value) {
        case_int(i) {
            fprintf(f, "%zi", i);
            break;
        }
        case_const(c) {
            fprintf(f, "%s", get_constant_name(c));
            break;
        }
        case_pair(p) {
            print_list(f, p);
            break;
        }
        case_sym(sym) {
            fprintf(f, "%s", symbol_name(sym));
            break;
        }
        default:
            fprintf(f, "#<%s value 0x%zx>", get_tag_name(value._tag), value._integer);
            break;
    }
}
