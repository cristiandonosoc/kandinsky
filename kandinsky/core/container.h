#pragma once

#include <kandinsky/core/defines.h>

#include <array>
#include <functional>
#include <span>

namespace kdk {

struct Arena;

// TODO(cdc): Replace with no allocation version.
template <typename T>
using Function = std::function<T>;

template <typename T>
std::span<T> MakeSpan(T& t) {
    return {&t, 1};
}

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

    Iterator(T* entities, i32 entity_count, i32 index)
        : _Entities(entities), _EntityCount(entity_count), _Index(index) {}

    T* _Entities = nullptr;
    i32 _EntityCount = 0;
    i32 _Index = 0;
};

// FixedArray --------------------------------------------------------------------------------------

template <typename T, i32 N>
struct FixedArray {
    using ElementType = T;
	static constexpr i32 kMaxSize = N;

    T Data[N] = {};
    i32 Size = 0;

    void Clear() { Size = 0; }

    std::span<T> ToSpan() { return {Data, (u32)Size}; }

    T& operator[](i32 index);
    const T& operator[](i32 index) const;

    T& First() { return Data[0]; }
    const T& First() const { return Data[0]; }

    T& Last() { return Data[Size - 1]; }
    const T& Last() const { return Data[Size - 1]; }

    T& At(i32 index) { return Data[index]; }
    const T& At(i32 index) const { return Data[index]; }

    T& Push(const T& elem);
    template <typename ITERATOR>
    i32 Push(ITERATOR begin, ITERATOR end);
    i32 Push(std::span<const T> span) { return Push(span.begin(), span.end()); }

    void Pop();

    bool IsFull() const { return Size >= N; }
    bool IsEmpty() const { return Size == 0; }
    i32 Capacity() const { return N; }

    std::pair<i32, T*> Find(const T& elem);
    std::pair<i32, const T*> Find(const T& elem) const;
    bool Contains(const T& elem) const { return Find(elem).first != NONE; }

    i32 Remove(const T& elem, i32 count = 1);
    i32 RemovePred(const Function<bool(const T&)>& pred, i32 count = 1);

    i32 RemoveAll(const T& elem) { return Remove(elem, 0); }
    i32 RemoveAllPred(const Function<bool(const T&)>& pred) { return RemovePred(pred, 0); }

    bool RemoveUnordered(const T& elem);
    bool RemoveUnorderedPred(const Function<bool(const T& elem)>& pred);
    void RemoveUnorderedAt(i32 index);

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

template <typename T, i32 N>
T& FixedArray<T, N>::operator[](i32 index) {
    ASSERT(index < Size);
    return Data[index];
}

template <typename T, i32 N>
const T& FixedArray<T, N>::operator[](i32 index) const {
    ASSERT(index < Size);
    return Data[index];
}

template <typename T, i32 N>
template <typename ITERATOR>
i32 FixedArray<T, N>::Push(ITERATOR begin, ITERATOR end) {
    i32 count = 0;
    for (auto it = begin; it != end && !IsFull(); ++it, ++count) {
        Push(*it);
    }
    return count;
}

template <typename T, i32 N>
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

template <typename T, i32 N>
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

template <typename T, i32 N>
std::pair<i32, T*> FixedArray<T, N>::Find(const T& elem) {
    auto [index, t] = const_cast<const FixedArray<T, N>*>(this)->Find(elem);
    return {index, const_cast<T*>(t)};
}

template <typename T, i32 N>
std::pair<i32, const T*> FixedArray<T, N>::Find(const T& elem) const {
    for (i32 i = 0; i < Size; i++) {
        if (Data[i] == elem) {
            return {i, &Data[i]};
        }
    }

    return {NONE, nullptr};
}

template <typename T, i32 N>
i32 FixedArray<T, N>::Remove(const T& elem, i32 count) {
    if (Size == 0) [[unlikely]] {
        return 0;  // Nothing to remove
    }

    if (count == 0) {
        count = N;
    }

    // Mark positions to remove with indices
    i32 write_pos = 0;  // Position to write next valid element
    i32 removed = 0;    // Count of elements marked for removal

    // First pass: identify elements to keep/remove
    for (i32 read_pos = 0; read_pos < Size; read_pos++) {
        if (removed < count && Data[read_pos] == elem) {
            // Element should be removed
            removed++;
        } else {
            // Element should be kept - move it if necessary
            if (write_pos != read_pos) {
                if constexpr (std::is_trivially_copyable_v<T>) {
                    Data[write_pos] = std::move(Data[read_pos]);
                } else {
                    Data[write_pos] = std::move(Data[read_pos]);
                    Data[read_pos].~T();
                }
            }
            write_pos++;
        }
    }

    // Update size
    Size = write_pos;
    return removed;
}

template <typename T, i32 N>
i32 FixedArray<T, N>::RemovePred(const Function<bool(const T&)>& pred, i32 count) {
    if (Size == 0) [[unlikely]] {
        return 0;  // Nothing to remove
    }

    if (count == 0) {
        count = N;
    }

    // Mark positions to remove with indices
    i32 write_pos = 0;  // Position to write next valid element
    i32 removed = 0;    // Count of elements marked for removal

    // First pass: identify elements to keep/remove
    for (i32 read_pos = 0; read_pos < Size; read_pos++) {
        if (removed < count && pred(Data[read_pos])) {
            // Element should be removed
            removed++;
        } else {
            // Element should be kept - move it if necessary
            if (write_pos != read_pos) {
                if constexpr (std::is_trivially_copyable_v<T>) {
                    Data[write_pos] = std::move(Data[read_pos]);
                } else {
                    Data[write_pos] = std::move(Data[read_pos]);
                    Data[read_pos].~T();
                }
            }
            write_pos++;
        }
    }

    // Update size
    Size = write_pos;
    return removed;
}

template <typename T, i32 N>
bool FixedArray<T, N>::RemoveUnordered(const T& elem) {
    if (Size == 0) [[unlikely]] {
        return false;
    }

    // Find the element to remove
    for (i32 i = 0; i < Size; i++) {
        if (Data[i] == elem) {
            RemoveUnorderedAt(i);
            return true;
        }
    }

    return false;  // Element not found
}

template <typename T, i32 N>
bool FixedArray<T, N>::RemoveUnorderedPred(const Function<bool(const T&)>& pred) {
    if (Size == 0) [[unlikely]] {
        return false;
    }

    // Find the element to remove
    for (i32 i = 0; i < Size; i++) {
        if (pred(Data[i])) {
            RemoveUnorderedAt(i);
            return true;
        }
    }

    return false;  // Element not found
}

template <typename T, i32 N>
void FixedArray<T, N>::RemoveUnorderedAt(i32 index) {
    ASSERT(index >= 0 && index < Size);

    // Move the last element to this position
    if (index != Size - 1) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            Data[index] = Data[Size - 1];
        } else {
            Data[index] = std::move(Data[Size - 1]);
            Data[Size - 1].~T();
        }
    } else if constexpr (!std::is_trivially_copyable_v<T>) {
        // If removing the last element and it's non-trivial, call destructor
        Data[index].~T();
    }

    Size--;
}

// DynArray ----------------------------------------------------------------------------------------

static constexpr i32 kDynArrayInitialCap = 4;

template <typename T>
struct DynArray {
    T* Base = nullptr;
    i32 Size = 0;
    i32 Cap = 0;

    T& operator[](i32 index);
    const T& operator[](i32 index) const;

    T& First() { return Base[0]; }
    const T& First() const { return Base[0]; }

    T& Last() { return Base[Size - 1]; }
    const T& Last() const { return Base[Size - 1]; }

    T& At(i32 index) { return Base[index]; }
    const T& At(i32 index) const { return Base[index]; }

    T& Push(Arena* arena, const T& elem);
    T Pop();
    void Reserve(Arena* arena, i32 new_cap);

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
DynArray<T> NewDynArray(Arena* arena, i32 initial_cap = kDynArrayInitialCap) {
    T* base = (T*)ArenaPush(arena, initial_cap * sizeof(T), alignof(T));
    return DynArray<T>{
        .Base = base,
        .Size = 0,
        .Cap = initial_cap,
    };
}

template <typename T>
T& DynArray<T>::operator[](i32 index) {
    ASSERT(index < Size);
    return Base[index];
}

template <typename T>
const T& DynArray<T>::operator[](i32 index) const {
    ASSERT(index < Size);
    return Base[index];
}

template <typename T>
T& DynArray<T>::Push(Arena* arena, const T& value) {
    if (Cap == 0) [[unlikely]] {
        ASSERT(Size == 0);
        Cap = kDynArrayInitialCap;
        Base = ArenaPushArray<T>(arena, Cap).data();
    }

    // Get more memory.
    if (Size == Cap) [[unlikely]] {
        Cap += Cap;
        T* new_base = ArenaPushArray<T>(arena, Cap).data();

        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(new_base, Base, Size * sizeof(T));
        } else {
            // Move each element to the new location
            for (i32 i = 0; i < Size; i++) {
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
void DynArray<T>::Reserve(Arena* arena, i32 new_cap) {
    if (new_cap <= Cap) {
        return;  // Already have enough capacity
    }

    T* new_base = ArenaPushArray<T>(arena, new_cap).data();

    if (Size > 0) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(new_base, Base, Size * sizeof(T));
        } else {
            // Move each element to the new location
            for (i32 i = 0; i < Size; i++) {
                new (new_base + i) T(std::move(Base[i]));
                Base[i].~T();
            }
        }
    }

    Base = new_base;
    Cap = new_cap;
}

}  // namespace kdk
