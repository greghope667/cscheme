#include "sxi.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/resource.h>

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

void add_defines(sxi::Environment* env) {
    sxi::env_define(env, sxi::make_symbol("+"),
        wrap(+[](int n, sxi::SXI* p) {
            sxi::sxi_int total = 0;
            for (int i=0; i<n; i++)
                total += as<sxi::sxi_int>(p[i]);
            return sxi::wrap(total);
        })
    );
    sxi::env_define(env, sxi::make_symbol("~"),
        wrap(+[](sxi::SXI v) {
            return sxi::wrap(~as<sxi::sxi_int>(v));
        })
    );
}

int main(int argc, char** argv) {
    auto options = options_s::parse(argc, argv);

    auto env = sxi::interaction_environment();
    add_defines(env);
    gc_protect(wrap(env));

    {
        rlimit limit{};
        getrlimit(RLIMIT_AS, &limit);
        limit.rlim_cur = 128 * 1024 * 1024;
        setrlimit(RLIMIT_AS, &limit);
        printf("RLIMIT_AS: cur %li max %li\n", limit.rlim_cur, limit.rlim_max);
    }

    if (options.expr) {
        auto stream = fmemopen((void*)options.expr, strlen(options.expr), "r");
        rep(stream, env);
        fclose(stream);
    } else {
        do { printf("> "); } while (rep(stdin, env));
    }
};
