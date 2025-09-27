#pragma once

#include <kandinsky/core/container.h>

#include <algorithm>

namespace kdk {

template <typename T, i32 N>
void Sort(FixedArray<T, N>* fa) {
    std::sort(fa->begin(), fa->end());
}

template <typename T, i32 N, typename PREDICATE>
void SortPred(FixedArray<T, N>* fa, const PREDICATE& pred) {
    std::sort(fa->begin(), fa->end(), pred);
}

}  // namespace kdk
