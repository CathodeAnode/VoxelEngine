#ifndef HELPERS_H
#define HELPERS_H

#include <intrin.h>


inline constexpr unsigned long BITS_IN_ULL = sizeof(unsigned long long) * 8;

inline unsigned long GetTrailingZeros(unsigned long long val)
{
    if (val == 0) return BITS_IN_ULL;

    unsigned long ret;
#ifdef _MSC_VER
    _BitScanForward64(&ret, val);
#else
    ret = __builtin_ctzll(val);
#endif
    return ret;
}

inline unsigned long GetTrailingOnes(unsigned long long val)
{
    if (val == 0) return 0;

    unsigned long ret;
#ifdef _MSC_VER
    _BitScanForward64(&ret, ~val);
#else
    ret = __builtin_ctzll(~val);
#endif
    return ret;
}




#endif