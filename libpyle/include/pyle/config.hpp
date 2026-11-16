#pragma once

#define PYLE_MODULE_ABI_VERSION 1

#if defined(__EMSCRIPTEN__)
    #define PYLE_HAS_DYNLIB 0
    #define PYLE_HAS_THREADS 0
    #define PYLE_HAS_PROCESS 0
#else
    #define PYLE_HAS_DYNLIB 1
    #define PYLE_HAS_THREADS 1
    #define PYLE_HAS_PROCESS 1
#endif

namespace pyle {
    #define PYLE_VERSION "0.2.0"
}
