#pragma once

#include <stdio.h>
#include <string_view>
#include <stdlib.h>
#include <string.h>

struct OutStream {
    OutStream() {
        f = open_memstream(&ptr, &len);
    };
    ~OutStream() {
        fclose(f);
        free(ptr);
    }
    OutStream(const OutStream& other) = delete;
    OutStream& operator=(const OutStream& other) = delete;

    std::string_view str() const {
        fflush(f);
        return { ptr, len };
    }

    FILE* f;
    char* ptr = 0;
    size_t len = 0;
};

struct InStream {
    InStream(const char* s) {
        f = fmemopen((void*)s, strlen(s), "r");
    }
    ~InStream() {
        fclose(f);
    }
    InStream(const InStream& other) = delete;
    InStream& operator=(const InStream& other) = delete;

    FILE* f;
};
