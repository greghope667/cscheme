#include "alloc.hpp"
#include "sxi.hpp"

#include <stdlib.h>
#include <string.h>

struct node { struct node* next; };
static struct node* free_list;
constexpr size_t NODE_SIZE = 64;

void* sxi_malloc(size_t size) {
    if (size <= NODE_SIZE) {
        if (free_list) {
            auto n = free_list;
            free_list = n->next;
            return n;
        } else {
            auto ptr = ::malloc(NODE_SIZE);
            if (not ptr) sxi::error("out of memory");
            return ptr;
        }
    } else {
        auto ptr = ::malloc(size);
        if (not ptr) sxi::error("out of memory");
        return ptr;
    }
}

void* sxi_realloc(void* p, size_t old_size, size_t new_size) {
    if (new_size == 0) {
        sxi_free(p, old_size);
        return nullptr;
    }
    if (old_size == 0) {
        return sxi_malloc(new_size);
    }

    if (old_size <= NODE_SIZE) {
        if (new_size <= NODE_SIZE) {
            return p;
        } else {
            auto ptr = ::malloc(new_size);
            if (not ptr) sxi::error("out of memory");
            memcpy(ptr, p, old_size);
            sxi_free(p, old_size);
            return ptr;
        }
    } else {
        if (new_size <= NODE_SIZE) {
            auto ptr = sxi_malloc(new_size);
            memcpy(ptr, p, new_size);
            sxi_free(p, old_size);
            return ptr;
        } else {
            auto ptr = ::realloc(p, new_size);
            if (not ptr) sxi::error("out of memory");
            return ptr;
        }
    }
}

void sxi_free(void* p, size_t size) {
    if (not p)
        return;
    if (size <= NODE_SIZE) {
        auto n = (node*)p;
        n->next = free_list;
        free_list = n;
    } else {
        ::free(p);
    }
}
