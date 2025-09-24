#include "sxi.hpp"
#include "tl.hpp"

#include <assert.h>
#include <stdlib.h>

enum token_tag {
    eof,
    lparen,
    rparen,
    quote,
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

static token read_hash(reader r) {
    switch (int ch = r.getc(); ch) {
        case EOF:   sxi::error_f("read error: unexpected eof");
        case 't':   return { .tag = boolean, .data = true };
        case 'f':   return { .tag = boolean, .data = false };
        default:    sxi::error_f("read error: illegal escape: '#%c'", ch);
    }
}

static token read_token(reader r) {
    char buffer[128];
    skip_space: int ch = r.getc();
    switch (ch) {
        case EOF:
            return { .tag = eof, .data=0 };
        case '(':
            return { .tag = lparen, .data=0 };
        case ')':
            return { .tag = rparen, .data=0 };
        case '#':
            return read_hash(r);
        case '\'':
            return { .tag = quote, .data=0 };
        case ' ': case '\n': case '\t': case '\r':
            goto skip_space;
    }
    if (not isident(ch))
        sxi::error_f("read error: unexpected character '%c'", ch);

    int len = read_identifier(r, ch, buffer);

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

static SXI read_list(reader r);
static SXI read_value(reader r);

static Symbol* const symbol_quote = sxi::make_symbol("quote");

static SXI read_quote_form(reader r, Symbol* quote) {
    return cons(wrap(quote), cons(read_value(r), c_null));
}

static SXI read_value(reader r, token first) {
    switch (first.tag) {
        case boolean:       return wrap_bool(first.data);
        case eof:           return c_eof;
        case lparen:        return read_list(r);
        case integer:       return wrap<sxi_int>(first.data);
        case ident:         return wrap<Symbol>((Symbol*)first.data);
        case quote:         return read_quote_form(r, symbol_quote);

        case rparen: case dot:
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
            case eof:
                error("read error: unexpected eof");
            case rparen:
                return head;
            case dot:
                if (tail == &head)
                    error("read error: headless dotted pair");
                *tail = read_value(r);
                if (read_token(r).tag != rparen)
                    error("read error: expected rparen");
                return head;
            default:
                auto p = make_pair(read_value(r, t), c_null);
                *tail = wrap(p);
                tail = &p->second;
        }
    }
}

// SXI sxi::read(FILE* f) {
//     return read_value({f});
// }
//
// extern "C" __attribute__((alias("sxi::read"))) SXI sxi_read(FILE* f);
extern "C" SXI sxi_read(void* f) {
    return read_value({(FILE*)f});
}
__attribute__((alias("sxi_read"))) SXI sxi::read(FILE* f);
