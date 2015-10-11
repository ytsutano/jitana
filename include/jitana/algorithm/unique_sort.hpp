#ifndef JITANA_UNIQUE_SORT_HPP
#define JITANA_UNIQUE_SORT_HPP

#include <algorithm>

namespace jitana {
    template <typename T>
    inline void unique_sort(T& vec)
    {
        std::sort(begin(vec), end(vec));
        vec.erase(std::unique(begin(vec), end(vec)), end(vec));
    }
}

#endif
