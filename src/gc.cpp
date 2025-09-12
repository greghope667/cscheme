#include "sxi.hpp"
#include <stdlib.h>

sxi::Pair* sxi::alloc_pair() {
    return new sxi::Pair;
}
