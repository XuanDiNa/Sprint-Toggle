#pragma once
#include <cstdio>
inline unsigned checks{}, failures{};
inline void Check(bool result, char const* label) {
    ++checks;
    if (!result) { ++failures; std::printf("FAIL: %s\n",label); }
}
inline int Finish(char const* suite) {
    std::printf("%s: %u checks, %u failures\n",suite,checks,failures);
    return failures ? 1 : 0;
}
