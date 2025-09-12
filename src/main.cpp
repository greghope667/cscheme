#include "sxi.hpp"
#include <assert.h>

int main() {
    for (int i=0; i<10; i++)
        print(stdout, wrap(sxi::gensym()));
    for (;;) {
        try {
            printf("> ");
            auto s = sxi::read(stdin);
            if (s == sxi::c_eof)
                return 0;
            sxi::print(stdout, s);
        } catch (sxi::Error e) {
            printf("Exception: %s\n", e.what());
        }
        putchar('\n');
    }
};
