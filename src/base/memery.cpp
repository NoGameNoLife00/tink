
#include <stdexcept>
#include "jemalloc/jemalloc.h"

// Global operators to be replaced by a linker when this file is
// a part of the build

void* operator new(size_t size) {
    void* p = je_malloc(size);
    if (!p) {
        throw std::bad_alloc();
    }
    return p;
}

void* operator new[](size_t size) {
    void* p = je_malloc(size);
    if (!p) {
        throw std::bad_alloc();
    }
    return p;
}

void operator delete(void* p) {
    if (p) {
        je_free(p);
    }
}

void operator delete[](void* p) {
    if (p) {
        je_free(p);
    }
}

