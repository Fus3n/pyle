#pragma once

#if defined(_MSC_VER)
    #define PYLE_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define PYLE_FORCEINLINE inline __attribute__((always_inline))
#else
    #define PYLE_FORCEINLINE inline
#endif