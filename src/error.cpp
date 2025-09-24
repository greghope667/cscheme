#include "error.hpp"

#include <stdio.h>
#include <stdarg.h>

static char error_buffer[4096];

[[noreturn]] void sxi::error(const char* msg) {
    snprintf(error_buffer, sizeof(error_buffer), "%s", msg);
    SXI_THROW;
}

[[noreturn]] void sxi::error(SXI obj, const char* msg) {
    FILE* f = fmemopen(error_buffer, sizeof(error_buffer), "w");
    fprintf(f, "%s: ", msg);
    print(f, obj);
    fclose(f);
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
    fprintf(
        f, "type error: expected %s, got %s ", 
        get_tag_name(expected), get_tag_name(actual._tag)
    );
    print(f, actual);
    fclose(f);
    SXI_THROW;
}

[[noreturn]] void sxi::invalid_arguments(SXI function, span<SXI> args) {
    FILE* f = fmemopen(error_buffer, sizeof(error_buffer), "w");
    fprintf(f, "invalid argument error:\nfunction: ");
    print(f, function);
    fprintf(f, "\ncalled with arguments: ( ");
    for (auto arg : args) {
        print(f, arg);
        fputc(' ', f);
    }
    fputc(')', f);
    fclose(f);
    SXI_THROW;
}

const char* sxi::Error::what() {
    return error_buffer;
}
