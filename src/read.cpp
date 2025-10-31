#include "sxi.hpp"
#include "tl.hpp"
#include "common.hpp"

#include <stdlib.h>

enum token_tag {
    t_eof,
    t_lparen,
    t_rparen,
    t_quote,
    t_boolean,
    t_dot,
    t_integer,
    t_ident,
    t_quasiquote,
    t_unquote,
    t_unquote_s,
    t_lambda,
    t_character,
    t_string,
    t_vector,
};

struct token {
    token_tag tag;
    intptr_t data;
};

static bool isident(unsigned char ch) {
    static constexpr auto cases = []{
        struct {
            bool ident[128] = {};
        } cases;
        for (int c = 'a'; c <= 'z'; c++) cases.ident[c] = true;
        for (int c = 'A'; c <= 'Z'; c++) cases.ident[c] = true;
        for (int c = '0'; c <= '9'; c++) cases.ident[c] = true;
        for (const char* p = "!$%&*+-./:<=>?@^_~"; *p; p++)
            cases.ident[(int)*p] = true;
        return cases;
    }();
    return ch < sizeof(cases) && cases.ident[ch];
}

struct reader {
    FILE* f;
    int getc() { return ::getc(f); }
    void ungetc(int ch) { ::ungetc(ch, f); }
};

static int read_identifier(reader r, char first, char out[sxi::MAX_SYMBOL_LENGTH]) {
    int size = 0;
    out[size++] = first;
    int ch;

    for (;;) {
        ch = r.getc();
        if (not isident(ch))
            break;
        if (ch == EOF)
            sxi::error_f("read error: unexpected eof");
        if (size > sxi::MAX_SYMBOL_LENGTH)
            sxi::error_f("read error: identifier too long: %.*s...", size, out);
        out[size++] = ch;
    }
    r.ungetc(ch);
    return size;
}

static sxi::character read_character(reader r) {
    int ch = r.getc();
    if (ch == EOF)
        sxi::error("read error: unexpected eof");
    if (ch < 'a' || ch > 'z')
        return ch;

    char buffer[sxi::MAX_SYMBOL_LENGTH];
    auto len = read_identifier(r, ch, buffer);

    if (len == 1)
        return ch;

    buffer[len] = 0;

    if (ch == 'x') {
        char* end;
        auto i = strtoll(buffer+1, &end, 16);
        if (end == &buffer[len] && 0 <= i && i < 256)
            return i;
    } else {
        for (auto [name, ch] : sxi::character_names)
            if (strcmp(name, buffer) == 0)
                return ch;
    }

    sxi::error_f("read error: unknown character escape sequence: \\%s", buffer);
}

static sxi::integer read_number(reader r, int base) {
    char buffer[sxi::MAX_SYMBOL_LENGTH];
    auto len = read_identifier(r, r.getc(), buffer);
    buffer[len] = 0;
    char* end;
    auto i = strtoll(buffer, &end, base);
    if (end != &buffer[len])
        sxi::error_f("read error: invalid number %s for base %i", buffer, base);
    return i;
}

static token read_hash(reader r) {
    switch (int ch = r.getc(); ch) {
        case EOF:   sxi::error_f("read error: unexpected eof");
        case 't':   return { .tag = t_boolean, .data = true };
        case 'f':   return { .tag = t_boolean, .data = false };
        case '\\':  return { .tag = t_character, .data = read_character(r) };
        case 'b':   return { .tag = t_integer, .data = read_number(r, 2) };
        case 'o':   return { .tag = t_integer, .data = read_number(r, 8) };
        case 'd':   return { .tag = t_integer, .data = read_number(r, 10) };
        case 'x':   return { .tag = t_integer, .data = read_number(r, 16) };
        case '(':   return { .tag = t_vector, .data = {} };
        default:    sxi::error_f("read error: illegal escape: '#%c'", ch);
    }
}

static void read_string_escape(reader r, sxi::String* str) {
    auto ch = r.getc();
    if (ch == ' ' || ch == '\r' || ch == '\t' || ch == '\n') {
        while (ch != '\n' && ch != EOF) ch = r.getc();
        return;
    }
    for (auto [escape, chr] : sxi::string_escapes) {
        if (ch == escape) {
            str->push(chr);
            return;
        }
    }
    sxi::error_f("read error: unknown string escape sequence: \\%c", ch);
}

static token read_string(reader r) {
    auto str = new sxi::String{};
    for (;;) {
        switch (int ch = r.getc(); ch) {
            case EOF:
                sxi::error("read error: unexpected eof");
            case '"':
                return { .tag = t_string, .data = (intptr_t)str };
            case '\\':
                read_string_escape(r, str);
                break;
            default:
                str->push(ch);
        }
    }
}

static token read_token(reader r) {
    char buffer[128];
    skip_space: int ch = r.getc();
    switch (ch) {
        case EOF:
            return { .tag = t_eof, .data=0 };
        case '(': case '[':
            return { .tag = t_lparen, .data=0 };
        case ')': case ']':
            return { .tag = t_rparen, .data=0 };
        case '#':
            return read_hash(r);
        case '\'':
            return { .tag = t_quote, .data=0 };
        case '`':
            return { .tag = t_quasiquote, .data=0 };
        case ',':
            if (ch = r.getc(); ch == '@') {
                return { .tag = t_unquote_s, .data=0 };
            } else {
                r.ungetc(ch);
                return { .tag = t_unquote, .data=0 };
            }
        case ' ': case '\n': case '\t': case '\r':
            goto skip_space;
        case ';':
            do { ch = r.getc(); } while (ch != EOF && ch != '\n');
            goto skip_space;
        case '\\': // extension: (\(args) body) -> (lambda (args) body)
            return { .tag = t_lambda, .data=0 };
        case '"':
            return read_string(r);
    }
    if (not isident(ch))
        sxi::error_f("read error: unexpected character '%c'", ch);

    int len = read_identifier(r, ch, buffer);

    if (len == 1 && buffer[0] == '.')
        return { .tag = t_dot, .data=0 };

    buffer[len] = 0;
    char* end;
    auto i = strtoll(buffer, &end, 10);
    if (end == &buffer[len])
        return { .tag = t_integer, .data = i };

    auto sym = sxi::make_symbol(buffer, len);
    return { .tag = t_ident, .data = (intptr_t)sym };
}

using namespace sxi;

static SXI read_list(reader r);
static SXI read_vector(reader r);
static SXI read_value(reader r);

static Symbol* const symbol_quote      = sxi::make_symbol("quote");
static Symbol* const symbol_quasiquote = sxi::make_symbol("quasiquote");
static Symbol* const symbol_unquote    = sxi::make_symbol("unquote");
static Symbol* const symbol_unquote_s  = sxi::make_symbol("unquote-splicing");
static Symbol* const symbol_lambda     = sxi::make_symbol("lambda");

static SXI read_quote_form(reader r, Symbol* quote) {
    return list({wrap(quote), read_value(r)});
}

static SXI read_value(reader r, token first) {
    switch (first.tag) {
        case t_boolean:     return wrap_bool(first.data);
        case t_eof:         return c_eof;
        case t_lparen:      return read_list(r);
        case t_integer:     return wrap<integer>(first.data);
        case t_ident:       return wrap((Symbol*)first.data);
        case t_quote:       return read_quote_form(r, symbol_quote);
        case t_quasiquote:  return read_quote_form(r, symbol_quasiquote);
        case t_unquote:     return read_quote_form(r, symbol_unquote);
        case t_unquote_s:   return read_quote_form(r, symbol_unquote_s);
        case t_lambda:      return wrap(symbol_lambda);
        case t_character:   return wrap(sxi::character(first.data));
        case t_string:      return wrap((sxi::String*)first.data);
        case t_vector:      return read_vector(r);

        case t_rparen: case t_dot:
            error("read error: unexpected token");
    }
    SXI_UNREACHABLE;
}

static SXI read_value(reader r) {
    return read_value(r, read_token(r));
}

static SXI read_list(reader r) {
    SXI head = c_null;
    auto tail = &head;

    for (;;) {
        switch (token t = read_token(r); t.tag) {
            case t_eof:
                error("read error: unexpected eof");
            case t_rparen:
                return head;
            case t_dot:
                if (tail == &head)
                    error("read error: headless dotted pair");
                *tail = read_value(r);
                if (read_token(r).tag != t_rparen)
                    error("read error: expected rparen");
                return head;
            default:
                auto p = make_pair(read_value(r, t), c_null);
                *tail = wrap(p);
                tail = &p->second;
        }
    }
}

static SXI read_vector(reader r) {
    auto vec = make_vector();
    for (;;) {
        switch (token t = read_token(r); t.tag) {
            case t_eof:
                error("read error: unexpected eof");
            case t_rparen:
                return wrap(vec);
            default:
                vector_push(vec, read_value(r, t));
        }
    }
}

SXI sxi::read(FILE* f) {
    return read_value({f});
}
