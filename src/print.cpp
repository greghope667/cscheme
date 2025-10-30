#include "code.hpp"
#include "env.hpp"
#include "sxi.hpp"
#include "match.hpp"
#include "string.hpp"
#include "struct.hpp"
#include "vector.hpp"

#include <stdio.h>

using namespace sxi;

const char* sxi::get_tag_name(sxi_tag tag) {
    switch (tag) {
    case SXI_TAG_const:     return "constant";
    case SXI_TAG_int:       return "integer";
    case SXI_TAG_sym:       return "symbol";
    case SXI_TAG_pair:      return "pair";
    case SXI_TAG_env:       return "environment";
    case SXI_TAG_function_n:return "function";
    case SXI_TAG_function_s:return "function";
    case SXI_TAG_function_1:return "function";
    case SXI_TAG_lambda:    return "lambda";
    case SXI_TAG_char:      return "character";
    case SXI_TAG_string:    return "string";
    case SXI_TAG_struct_type:       return "struct type";
    case SXI_TAG_struct_instance:   return "struct instance";
    case SXI_TAG_vector:    return "vector";
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
    case SXI_CONST_void:    return "#void";
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

static void print_formals(FILE* f, Formals* formals) {
    fprintf(f, "(");
    for (int i=0; i<formals->names.length-1; i++) {
        fprintf(f, "%s ", symbol_name(formals->names[i]));
    }
    if (formals->is_variadic)
        fprintf(f, ". ");
    if (formals->names.length > 0)
        fprintf(f, "%s", symbol_name(formals->names[formals->names.length-1]));
    fprintf(f, ")");
}

static void print_character(FILE* f, character ch) {
    if (' ' < ch && ch < 0x7f) {
        fprintf(f, "#\\%c", ch);
        return;
    }

    if (ch <= ' ' || ch == 0x7f) {
        for (auto [name, chr] : character_names) {
            if (ch == chr) {
                fprintf(f, "#\\%s", name);
                return;
            }
        }
    }

    fprintf(f, "#\\x%x", ch);
}

static void print_string_character(FILE* f, sxi::character ch) {
    if (' ' <= ch && ch < 0x7f) {
        fputc(ch, f);
        return;
    }
    if (ch < ' ') {
        for (auto [escape, chr] : string_escapes) {
            if (ch == chr) {
                fprintf(f, "\\%c", escape);
                return;
            }
        }
    }
    fprintf(f, "\\x%x", ch);
}

static void print_string(FILE* f, String* str) {
    fputc('"', f);
    for (auto ch : *str) print_string_character(f, ch);
    fputc('"', f);
}

static void print_struct_type(FILE* f, StructType* st) {
    fprintf(f, "#<struct-type %s ( ", symbol_name(st->name));
    for (auto name : st->field_names)
        fprintf(f, "%s ", symbol_name(name));
    fprintf(f, ")>");
}

static void print_struct(FILE* f, StructInstance* si) {
    fprintf(f, "#<%s", symbol_name(si->type->name));
    for (int i=0; i<si->fields.length; i++) {
        fprintf(f, " %s=", symbol_name(si->type->field_names[i]));
        print(f, si->fields[i]);
    }
    fprintf(f, ">");
}

static void print_vector(FILE* f, Vector* vec) {
    if (vec->length == 0) {
        fprintf(f, "#()");
        return;
    }
    fprintf(f, "#(");
    for (auto p = vec->begin(), final = vec->end()-1; ; p++) {
        print(f, *p);
        if (p == final) {
            fputc(')', f);
            break;
        } else {
            fputc(' ', f);
        }
    }
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
        case_lambda(lambda) {
            if (lambda->code->name)
                fprintf(f, "#<lambda %s ", symbol_name(lambda->code->name));
            else
                fprintf(f, "#<lambda %p ", lambda);
            print_formals(f, lambda->arguments);
            fputc('>', f);
            break;
        }
        case_val(sxi::character, ch) {
            print_character(f, ch);
            break;
        }
        case_ptr(String, str) {
            print_string(f, str);
            break;
        }
        case_ptr(StructType, st) {
            print_struct_type(f, st);
            break;
        }
        case_ptr(StructInstance, si) {
            print_struct(f, si);
            break;
        }
        case_ptr(Vector, v) {
            print_vector(f, v);
            break;
        }
        case SXI_TAG_function_1:
        case SXI_TAG_function_n:
        case SXI_TAG_function_s:
            if (auto name = get_function_name(value._integer))
                fprintf(f, "#<function %s>", name);
            else
                fprintf(f, "#<function 0x%zx>", value._integer);
            break;
        default:
            fprintf(f, "#<%s 0x%zx>", get_tag_name(value._tag), value._integer);
            break;
    }
}

const char* sxi::get_opcode_name(opcode op) {
    switch (op) {
        #define X(op) case op: return #op;
        OPCODES
        #undef X
    }
    error_f("opcode %i has no name\n", op);
}

static void disassemble_code(FILE* f, Code* code) {
    auto len = code->insns_len;
    fprintf(
        f, "CODE %p: (len %i sym %i lit %i lambda %i)\n",
        code, len, code->symbols_len, code->literals_len, code->lambdas_len
    );
    int ip = 0;
    auto insns = code->insns;
    while (ip < len) {
        auto op = insns[ip];
        printf("%8i:  %-24s", ip, get_opcode_name(op)+3);

        switch (op) {
            case op_literal:
                print(f, code->literals[insns[ip+1]]);
                ip += 2;
                break;

            case op_lambda: {
                auto lambda = code->lambdas[insns[ip+1]];
                fprintf(f, "#<protolambda %p ", lambda.code);
                print_formals(f, lambda.arguments);
                fprintf(f, ">");
                ip += 2;
                break;
            }

            case op_set:
            case op_define:
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

            case op_branch:
            case op_branch0: {
                auto dist = insns[ip+1];
                fprintf(f, "%+i <%i>", dist, ip + dist);
                ip += 2;
                break;
            }

            case op_alloc_cont:
            case op_call:
            case op_tailcall:
            case op_ret:
            case op_push:
            case op_exit:
            case op_unlambda:
                ip += 1;
                break;
        }
        putchar('\n');
    }

    for (int i=0; i<code->lambdas_len; i++) {
        disassemble_code(f, code->lambdas[i].code);
    }
}

void sxi::disassemble(FILE* f, const Lambda* l) {
    disassemble_code(f, l->code);
}
