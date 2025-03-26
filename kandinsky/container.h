#pragma once

#include <kandinsky/memory.h>

#include <array>

namespace kdk {

// FixedArray --------------------------------------------------------------------------------------

template <typename T, u32 N>
struct FixedArray {
    T Data[N] = {};
    u32 Count = 0;

    T& operator[](u32 index);
    const T& operator[](u32 index) const;

    T& First() { return Data[0]; }
    const T& First() const { return Data[0]; }

    T& Last() { return Data[Count - 1]; }
    const T& Last() const { return Data[Count - 1]; }

    bool Push(const T& elem);
    T Pop();

    bool IsFull() const { return Count >= N; }
    bool IsEmpty() const { return Count == 0; }
    u32 Capacity() const { return N; }
};
// Debug which requirements are failing
static_assert(std::is_trivially_copyable_v<FixedArray<const char*, 4>>, "Not trivially copyable");
static_assert(std::is_standard_layout_v<FixedArray<const char*, 4>>, "Not standard layout");

template <typename T, u32 N>
T& FixedArray<T, N>::operator[](u32 index) {
    ASSERT(index < Count);
    return Data[index];
}

template <typename T, u32 N>
const T& FixedArray<T, N>::operator[](u32 index) const {
    ASSERT(index < Count);
    return Data[index];
}

template <typename T, u32 N>
bool FixedArray<T, N>::Push(const T& elem) {
    if (Count >= N) {
        return false;  // FixedArray is full
    }

    T* ptr = Data + Count;
    Count++;

    if constexpr (std::is_pod_v<T>) {
        *ptr = elem;
    } else {
        // For non POD things, we want to make sure the memory will not do weird things.
        std::memset(ptr, 0, sizeof(T));
        new (ptr) T(elem);  // Placement new.
    }

    return true;
}

template <typename T, u32 N>
T FixedArray<T, N>::Pop() {
    if (Count == 0) [[unlikely]] {
        return T{};
    }

    T& elem = Data[Count - 1];
    Count--;

    if constexpr (std::is_pod_v<T>) {
        return elem;
    }

    // For non-POD, we want to copy the result and destroy the original.
    T copy = elem;
    elem.~T();  // In-place destructor.
    return copy;
}

// DynArray ----------------------------------------------------------------------------------------

static constexpr u32 kDynArrayInitialCap = 4;

template <typename T>
struct DynArray {
    T* Base = nullptr;
    u32 Size = 0;
    u32 Cap = 0;

    T& operator[](u32 index);
    const T& operator[](u32 index) const;

    T& First() { return Base[0]; }
    const T& First() const { return Base[0]; }

    T& Last() { return Base[Size - 1]; }
    const T& Last() const { return Base[Size - 1]; }

    T& Push(Arena* arena, const T& elem);
    T Pop();
    void Reserve(Arena* arena, u32 new_cap);
};
static_assert(sizeof(DynArray<int>) == 16);

template <typename T>
DynArray<T> NewDynArray(Arena* arena, u32 initial_cap = kDynArrayInitialCap) {
    T* base = (T*)ArenaPush(arena, initial_cap * sizeof(T), alignof(T));
    return DynArray<T>{
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
T& DynArray<T>::Push(Arena* arena, const T& value) {
    if (Cap == 0) [[unlikely]] {
        ASSERT(Size == 0);
        Cap = kDynArrayInitialCap;
        Base = ArenaPushArray<T>(arena, Cap);
    }

    // Get more memory.
    if (Size == Cap) [[unlikely]] {
        Cap += Cap;
        T* new_base = ArenaPushArray<T>(arena, Cap);

        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(new_base, Base, Size * sizeof(T));
        } else {
            // Move each element to the new location
            for (u32 i = 0; i < Size; i++) {
                new (new_base + i) T(std::move(Base[i]));
                Base[i].~T();
            }
        }
        Base = new_base;
    }

    T* ptr = Base + Size;
    if constexpr (std::is_trivially_copyable_v<T>) {
        *ptr = value;
    } else {
        new (ptr) T(std::move(value));
    }
    Size++;
    return *ptr;
}

template <typename T>
T DynArray<T>::Pop() {
    if (Size == 0) [[unlikely]] {
        return T{};
    }

    T& elem = Base[Size - 1];
    Size--;

    if constexpr (std::is_trivially_copyable_v<T>) {
        return elem;
    } else {
        T result = std::move(elem);
        elem.~T();
        return result;
    }
}

template <typename T>
void DynArray<T>::Reserve(Arena* arena, u32 new_cap) {
    if (new_cap <= Cap) {
        return;  // Already have enough capacity
    }

    T* new_base = ArenaPushArray<T>(arena, new_cap);

    if (Size > 0) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(new_base, Base, Size * sizeof(T));
        } else {
            // Move each element to the new location
            for (u32 i = 0; i < Size; i++) {
                new (new_base + i) T(std::move(Base[i]));
                Base[i].~T();
            }
        }
    }

    Base = new_base;
    Cap = new_cap;
}

}  // namespace kdk
