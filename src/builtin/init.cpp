#include "sxi.hpp"
#include "../tl.hpp"
#include "builtin.hpp"

using namespace sxi;

const char init_1[] = {
    #embed "init-1.scm"
};

const char init_2[] = {
    #embed "init-2.scm"
};

const char init_4[] = {
    #embed "init-4.scm"
};

constexpr span<const char> init_files[] = {
    { init_1, sizeof(init_1 ) },
    { init_4, sizeof(init_4 ) },
    { init_2, sizeof(init_2 ) },
};

Environment* sxi::run_init_scm_code() {
    auto env = make_environment_rootlet();

    for (auto [data, len] : init_files) {
        FILE* f = fmemopen((void*)data, len, "rb");
        SXI_TRY {
            for (;;) {
                auto expr = read(f);
                if (expr == c_eof)
                    break;
                eval(expr, env);
            }
        } SXI_CATCH(e) {
            fclose(f);
            fprintf(stderr, "Exception during initialisation: %s\n", e.what());
        }
    }

    return env;
}
