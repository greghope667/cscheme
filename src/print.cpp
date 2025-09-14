#include "code.hpp"
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
        if (not instance<Pair>(rest)) {
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

const char* sxi::get_opcode_name(opcode op) {
    switch (op) {
        case op_literal:        return "literal";
        case op_ret:            return "ret";
        case op_lookup:         return "lookup";
        case op_alloc_cont:     return "alloc_cont";
        case op_alloc_stack:    return "alloc_stack";
        case op_push:           return "push";
        case op_call:           return "call";
        case op_tailcall:       return "tailcall";
        case op_exit:           return "exit";
    }
    static char opname[32];
    snprintf(opname, 32, "<op %i>", op);
    fprintf(stderr, "WARNING: opcode %s has no name\n", opname);
    return opname;
}

static void disassemble_code(FILE* f, Code* code) {
    auto len = code->insns_len;
    fprintf(
        f, "CODE: (len %i sym %i lit %i) %p\n", 
        len, code->symbols_len, code->literals_len, code
    );
    int ip = 0;
    auto insns = code->insns;
    while (ip < len) {
        auto op = insns[ip];
        printf("%8i:  %-24s", ip, get_opcode_name(op));

        switch (op) {
            case op_literal:
                print(f, code->literals[insns[ip+1]]);
                ip += 2;
                break;
            
            case op_lookup: {
                auto sym = code->symbols[insns[ip+1]];
                fprintf(f, "%s", symbol_name(sym));
                ip += 2;
                break;
            }

            case op_alloc_stack:
                fprintf(f, "%i", insns[ip+1]);
                ip += 2;
                break;

            case op_alloc_cont:
            case op_call:
            case op_tailcall:
            case op_ret:
            case op_push:
            case op_exit:
                ip += 1;
                break;
        }
        putchar('\n');
    }
}

void sxi::disassemble(FILE* f, const Thunk* t) { 
    disassemble_code(f, t->code);
}

void sxi::disassemble(FILE* f, const Lambda* l) {
    disassemble_code(f, l->code);
}