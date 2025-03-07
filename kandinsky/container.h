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

    T& operator[](u32 index);
    const T& operator[](u32 index) const;

    T& First() { return Base[0]; }
    const T& First() const { return Base[0]; }

    T& Last() { return Base[Size - 1]; }
    const T& Last() const { return Base[Size - 1]; }

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
T& DynArray<T>::operator[](u32 index) {
    ASSERT(index < Size);
    return Base[index];
}

template <typename T>
const T& DynArray<T>::operator[](u32 index) const {
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
    if constexpr (std::is_pod_v<T>) {
        *ptr = elem;
        return *ptr;
    }

    // For non POD things, we want to make sure the memory will not weird things.
    std::memset(ptr, 0, sizeof(T));
    new (ptr) T(elem);  // Placement new.
    return *ptr;
}

template <typename T>
T DynArray<T>::Pop() {
    if (Size == 0) [[unlikely]] {
        return T{};
    }

    T& elem = Base[Size - 1];
    Size--;
    if constexpr (std::is_pod_v<T>) {
        return elem;
    }

    // For non-POD, we want to copy the result and destroy the original.
    T copy = elem;
    elem.~T();  // In-place destructor.
    return copy;
}

}  // namespace kdk
