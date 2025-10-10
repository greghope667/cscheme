#include "sxi.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/resource.h>

static bool rep(FILE* f, sxi::Environment* env, bool print) {
    SXI_TRY {
        auto r = sxi::read(f);
        if (r == sxi::c_eof)
            return false;
        auto e = sxi::eval(r, env);
        if (print && not (e == sxi::c_void)) {
            sxi::print(stdout, e);
            putchar('\n');
        }
    } SXI_CATCH(e) {
        printf("Exception: %s\n", e.what());
    }
    return true;
}

struct options_s {
    const char* expr;
    const char* script;

    static options_s parse(int argc, char** argv) {
        int opt;
        options_s options = {};
        while ((opt = getopt(argc, argv, "c:s:")) != -1) {
            switch (opt) {
                case 'c':
                    options.expr = optarg;
                    break;
                case 's':
                    options.script = optarg;
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

    auto env = sxi::interaction_environment();
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
        rep(stream, env, true);
        fclose(stream);
    } else if (options.script) {
        auto stream = fopen(options.script, "rb");
        if (not stream) {
            perror("Unable to open script file");
            return EXIT_FAILURE;
        }
        while (rep(stream, env, false)) {};
        fclose(stream);
    } else {
        do { printf("> "); } while (rep(stdin, env, true));
    }
};
