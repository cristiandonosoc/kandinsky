#pragma once

#include <kandinsky/memory.h>

#include <array>
#include <functional>

namespace kdk {

// TODO(cdc): Replace with no allocation version.
template <typename R, typename... Args>
using Function = std::function<R(Args...)>;

// Iterator ----------------------------------------------------------------------------------------

template <typename T>
struct Iterator {
    Iterator() = default;

    T& Get() { return _Entities[_Index]; }
    T* GetPtr() { return _Entities + _Index; }
    T& operator*() { return _Entities[_Index]; }
    T* operator->() { return &_Entities[_Index]; }

    operator bool() const { return _Index < _EntityCount; }
    void operator++() { _Index++; }
    void operator++(int) { _Index++; }

    Iterator(T* entities, u32 entity_count, u32 index)
        : _Entities(entities), _EntityCount(entity_count), _Index(index) {}

    T* _Entities = nullptr;
    u32 _EntityCount = 0;
    u32 _Index = 0;
};

// Span --------------------------------------------------------------------------------------------

template <typename T>
struct Span {
    T* Entries = nullptr;
    u32 Size = 0;

    T& operator[](u32 index);
    const T& operator[](u32 index) const;

    // Iterator support
    T* begin() { return Entries; }
    T* end() { return Entries + Size; }
    const T* begin() const { return Entries; }
    const T* end() const { return Entries + Size; }
    const T* cbegin() const { return Entries; }
    const T* cend() const { return Entries + Size; }
};

template <typename T>
inline bool IsValid(const Span<T>& a) {
    return a.Entries != nullptr && a.Size > 0;
}

template <typename T>
T& Span<T>::operator[](u32 index) {
    ASSERT(index < Size);
    return Entries[index];
}

template <typename T>
const T& Span<T>::operator[](u32 index) const {
    ASSERT(index < Size);
    return Entries[index];
}

// FixedArray --------------------------------------------------------------------------------------

template <typename T, u32 N>
struct FixedArray {
    using ElementType = T;

    T Data[N] = {};
    u32 Size = 0;

    void Clear() { Size = 0; }

    T& operator[](u32 index);
    const T& operator[](u32 index) const;

    T& First() { return Data[0]; }
    const T& First() const { return Data[0]; }

    T& Last() { return Data[Size - 1]; }
    const T& Last() const { return Data[Size - 1]; }

    T& Get(u32 index) { return Data[index]; }
    const T& Get(u32 index) const { return Data[index]; }

    T& Push(const T& elem);
    void Pop();

    bool IsFull() const { return Size >= N; }
    bool IsEmpty() const { return Size == 0; }
    u32 Capacity() const { return N; }

    // Iterator support
    T* begin() { return Data; }
    T* end() { return Data + Size; }
    const T* begin() const { return Data; }
    const T* end() const { return Data + Size; }
    const T* cbegin() const { return Data; }
    const T* cend() const { return Data + Size; }
};
// Debug which requirements are failing
static_assert(std::is_trivially_copyable_v<FixedArray<const char*, 4>>, "Not trivially copyable");
static_assert(std::is_standard_layout_v<FixedArray<const char*, 4>>, "Not standard layout");

template <typename T, u32 N>
T& FixedArray<T, N>::operator[](u32 index) {
    ASSERT(index < Size);
    return Data[index];
}

template <typename T, u32 N>
const T& FixedArray<T, N>::operator[](u32 index) const {
    ASSERT(index < Size);
    return Data[index];
}

template <typename T, u32 N>
T& FixedArray<T, N>::Push(const T& elem) {
    ASSERT(!IsFull());
    T* ptr = Data + Size;
    Size++;

    if constexpr (std::is_pod_v<T>) {
        *ptr = elem;
    } else {
        // For non POD things, we want to make sure the memory will not do weird things.
        std::memset(ptr, 0, sizeof(T));
        new (ptr) T(elem);  // Placement new.
    }

    return Last();
}

template <typename T, u32 N>
void FixedArray<T, N>::Pop() {
    if (Size == 0) [[unlikely]] {
        return;
    }

    T& elem = Data[Size - 1];
    Size--;

    if constexpr (!std::is_trivially_copyable_v<T>) {
        // For non-POD, we want to copy the result and destroy the original.
        elem.~T();  // In-place destructor.
    }
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

    void Clear() { Size = 0; }

    // Iterator support
    T* begin() { return Base; }
    T* end() { return Base + Size; }
    const T* begin() const { return Base; }
    const T* end() const { return Base + Size; }
    const T* cbegin() const { return Base; }
    const T* cend() const { return Base + Size; }
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
