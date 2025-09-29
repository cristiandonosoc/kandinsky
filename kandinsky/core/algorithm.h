#pragma once

#include <kandinsky/core/container.h>

#include <algorithm>

namespace kdk {

template <typename T, i32 N>
void Sort(FixedVector<T, N>* fa) {
    std::sort(fa->begin(), fa->end());
}

template <typename T, i32 N, typename PREDICATE>
void SortPred(FixedVector<T, N>* fa, const PREDICATE& pred) {
    std::sort(fa->begin(), fa->end(), pred);
}

}  // namespace kdk
