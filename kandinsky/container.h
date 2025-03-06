#pragma once

#include <kandinsky/memory.h>

namespace kdk {

// DynArray ----------------------------------------------------------------------------------------

static constexpr u32 kDynArrayInitialCap = 4;

template <typename T>
struct DynArray {
    Arena* _Arena = nullptr;
    T* Base = nullptr;
    u32 Size = 0;
    u32 Cap = 0;

    T& operator[](u32 index) const;
    T& Push(const T& elem);
    T Pop();
};
static_assert(sizeof(DynArray<int>) == 24);

template <typename T>
DynArray<T> NewDynArray(Arena* arena, u32 initial_cap = kDynArrayInitialCap) {
    T* base = (T*)ArenaPush(arena, initial_cap * sizeof(T), alignof(T));
    return DynArray<T>{
        ._Arena = arena,
        .Base = base,
        .Size = 0,
        .Cap = initial_cap,
    };
}

template <typename T>
T& DynArray<T>::operator[](u32 index) const {
    ASSERT(index < Size);
    return Base[index];
}

template <typename T>
T& DynArray<T>::Push(const T& elem) {
    if (Size == Cap) [[unlikely]] {
        // Get more memory.
        ArenaPush(_Arena, Cap * sizeof(T), alignof(T));
        Cap += Cap;
    }

    T* ptr = Base + Size;
    Size++;
    // For non POD things, we want to make sure the memory will not weird things.
    if constexpr (!std::is_pod_v<T>) {
        std::memset(ptr, 0, sizeof(T));
    }
    *ptr = elem;
    return *ptr;
}

template <typename T>
T DynArray<T>::Pop() {
    if (Size == 0) [[unlikely]] {
        return T{};
    }

    T& elem = Base[Size - 1];
    Size--;
    return elem;
}

}  // namespace kdk
