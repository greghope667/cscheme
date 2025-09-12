#include "sxi.hpp"

#include <assert.h>
#include <stdlib.h>

enum token_tag {
    eof,
    lparen,
    rparen,
    // quote,
    boolean,
    dot,
    integer,
    ident,
    // quasiquote,
    // unquote,
    // unquote_splicing,
};

struct token {
    token_tag tag;
    intptr_t data;
};

static bool isident(char ch) {
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
    return ch < sizeof(cases) && cases.ident[(int)ch];
}


static FILE* read_input;

static int read_getc() {
    return getc(read_input);
}

static void read_ungetc(int ch) {
    ungetc(ch, read_input);
}

static int read_identifier(char first, char out[sxi::MAX_SYMBOL_LENGTH]) {
    int size = 0;
    out[size++] = first;
    int ch;

    for (;;) {
        ch = read_getc();
        if (not isident(ch))
            break;
        if (ch == EOF)
            sxi::error_f("read error: unexpected eof");
        if (size > sxi::MAX_SYMBOL_LENGTH)
            sxi::error_f("read error: identifier too long: %.*s...", size, out);
        out[size++] = ch;
    }
    read_ungetc(ch);
    return size;
}

static token read_hash() {
    int ch = read_getc();
    switch (ch) {
        case EOF:   sxi::error_f("read error: unexpected eof");
        case 't':   return { .tag = boolean, .data = true };
        case 'f':   return { .tag = boolean, .data = false };
        default:    sxi::error_f("read error: illegal escape: '#%c'", ch);
    }
}

static token read_token() {
    char buffer[128];
    skip_space: int ch = read_getc();
    switch (ch) {
        case EOF:
            return { .tag = eof, .data=0 };
        case '(':
            return { .tag = lparen, .data=0 };
        case ')':
            return { .tag = rparen, .data=0 };
        case '#':
            return read_hash();
        case ' ': case '\n': case '\t': case '\r':
            goto skip_space;
    }
    if (not isident(ch))
        sxi::error_f("read error: unexpected character '%c'", ch);

    int len = read_identifier(ch, buffer);

    if (len == 1 && buffer[0] == '.')
        return { .tag = dot, .data=0 };

    buffer[len] = 0;
    char* end;
    auto i = strtoll(buffer, &end, 10);
    if (end == &buffer[len])
        return { .tag = integer, .data = i };

    auto sym = sxi::make_symbol(buffer, len);
    return { .tag = ident, .data = (intptr_t)sym };
}

using namespace sxi;

static SXI read_list();

static SXI read_value(token first) {
    switch (first.tag) {
        case boolean:       return wrap_bool(first.data);
        case eof:           return c_eof;
        case lparen:        return read_list();
        case integer:       return wrap<sxi_int>(first.data);
        case ident:         return wrap<Symbol>((Symbol*)first.data);

        case rparen: case dot:
            error("read error: unexpected token");
    }
    assert(false);
}

static SXI read_value() {
    return read_value(read_token());
}

static SXI read_list() {
    SXI head = c_null;
    auto tail = &head;

    for (;;) {
        token t = read_token();
        switch (t.tag) {
            case eof:
                error("read error: unexpected eof");
            case rparen:
                return head;
            case dot:
                if (tail == &head)
                    error("read error: headless dotted pair");
                *tail = read_value();
                if (read_token().tag != rparen)
                    error("read error: expected rparen");
                return head;
            default:
                auto p = make_pair(read_value(t), c_null);
                *tail = wrap(p);
                tail = &p->second;
        }
    }
}

SXI sxi::read(FILE* f) {
    read_input = f;
    return read_value();
}
