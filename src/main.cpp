#include "sxi.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool rep(FILE* f, sxi::Environment* env) {
    SXI_TRY {
        auto r = sxi::read(f);
        if (r == sxi::c_eof)
            return false;
        sxi::print(stdout, r);
        putchar('\n');
        auto c = sxi::compile(r, env);
        sxi::disassemble(stdout, c);
        auto e = sxi::execute(c);
        sxi::print(stdout, e);
    } SXI_CATCH(e) {
        printf("Exception: %s", e.what());
    }
    putchar('\n');
    return true;
}

struct options_s {
    const char* expr;

    static options_s parse(int argc, char** argv) {
        int opt;
        options_s options = {};
        while ((opt = getopt(argc, argv, "c:")) != -1) {
            switch (opt) {
                case 'c':
                    options.expr = optarg;
                    break;
                default:
                    exit(EXIT_FAILURE);
            }
        } 
        return options;
    }
};

int main(int argc, char** argv) {
    auto options = options_s::parse(argc, argv);

    auto env = sxi::make_environment(nullptr);
    if (options.expr) {
        auto stream = fmemopen((void*)options.expr, strlen(options.expr), "r");
        rep(stream, env);
        fclose(stream);
    } else {
        do { printf("> "); } while (rep(stdin, env));
    }
};
