#pragma once

#include <utility>

namespace kdk {
namespace defer {

template <typename T>
class DeferContainer {
    // No copy.
    DeferContainer(const DeferContainer&) = delete;
    DeferContainer& operator=(const DeferContainer&) = delete;

    // No move.
    DeferContainer(DeferContainer&&) = delete;
    DeferContainer& operator=(DeferContainer&&) = delete;

   public:
    explicit DeferContainer(T&& fn) : Fn(std::move(fn)) {}
    ~DeferContainer() { Fn(); }

   private:
    T Fn;
};

// Permits to write the DEFER macro.
struct DeferSyntaxSupport {
    template <typename T>
    DeferContainer<T> operator+(T&& fn) {
        return DeferContainer<T>((T&&)std::move(fn));
    }
};

}  // namespace defer
}  // namespace kdk

#define DEFER_PRIVATE_JOIN(A, B) DEFER_PRIVATE_JOIN_INNER(A, B)
#define DEFER_PRIVATE_JOIN_INNER(A, B) A##B

#define DEFER                                                \
    const auto DEFER_PRIVATE_JOIN(__defer_guard, __LINE__) = \
        ::kdk::defer::DeferSyntaxSupport() + [&]()
