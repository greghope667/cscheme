#include "sxi.hpp"
#include <stdio.h>

int main() {
    for (;;) {
        SXI_TRY {
            printf("> ");
            auto s = sxi::read(stdin);
            if (s == sxi::c_eof)
                return 0;
            sxi::print(stdout, s);
        } SXI_CATCH(e) {
            printf("Exception %s", e.what());
        }
        putchar('\n');
    }
};
