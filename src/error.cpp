#include "sxi.hpp"
#include <stdio.h>
#include <stdarg.h>

static char error_buffer[4096];

[[noreturn]] void sxi::error(const char* msg) {
    snprintf(error_buffer, sizeof(error_buffer), "%s", msg);
    SXI_THROW;
}

[[noreturn]] void sxi::error_f(const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    vsnprintf(error_buffer, sizeof(error_buffer), msg, args);
    va_end(args);
    SXI_THROW;
}

[[noreturn]] void sxi::type_error(sxi_tag expected, SXI actual) {
    FILE* f = fmemopen(error_buffer, sizeof(error_buffer), "w");
    fprintf(f, "type error: expected %s, actual: ", get_tag_name(expected));
    print(f, actual);
    fclose(f);
    SXI_THROW;
}

const char* sxi::Error::what() {
    return error_buffer;
}
