#pragma once

#include <cstdint>
#include <utility>

// Detect compiler
#if defined(_MSC_VER)
#ifndef COMPILER_MSVC
#define COMPILER_MSVC
#endif
#elif defined(__clang__)
#define COMPILER_CLANG
#elif defined(__GNUC__)
#define COMPILER_GCC
#else
#error "Unknown compiler"
#endif

// Detect platform
#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS
#else
#error "Unsupported platform"
#endif

#ifdef KDK_ATTRIBUTE_GENERATION
#define KDK_ATTR(...) __attribute__((annotate("KDK", __VA_ARGS__)))
#else
#define KDK_ATTR(...)
#endif

// Define debug break macro
#if defined(COMPILER_MSVC)
// MSVC specific debug break
#define DEBUG_BREAK() __debugbreak()
#elif defined(PLATFORM_WINDOWS) && (defined(COMPILER_CLANG) || defined(COMPILER_GCC))
// Clang and GCC on Windows
#define DEBUG_BREAK() __asm__ volatile("int $3")
#elif defined(PLATFORM_LINUX) && (defined(COMPILER_CLANG) || defined(COMPILER_GCC))
// Clang and GCC on Linux
#include <signal.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#else
#error "Debug break not implemented for this platform/compiler combination"
#endif

#define NONE -1

#define KDK_DLL_EXPORT __

// Extra digits if needed: 3.1415926535897932384626433832795f
#define PI (3.1415926535897932f)
#define SMALL_NUMBER (1.0e-8)
#define KINDA_SMALL_NUMBER (1.e-4f)
#define BIG_NUMBER (3.4e+38f)

using i8 = int8_t;
using u8 = uint8_t;

using i16 = int16_t;
using u16 = uint16_t;

using i32 = int32_t;
using u32 = uint32_t;

using i64 = int64_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

// TODO(cdc): Make these no-op on non-debug builds.
#define ASSERT(expr)                                                            \
    do {                                                                        \
        if (!(expr)) {                                                          \
            ::kdk::internal::HandleAssert(#expr, __FILE__, __LINE__, __func__); \
        }                                                                       \
    } while (0)
#define ASSERTF(expr, fmt, ...)                                                                    \
    do {                                                                                           \
        if (!(expr)) {                                                                             \
            ::kdk::internal::HandleAssertf(#expr, __FILE__, __LINE__, __func__, fmt, __VA_ARGS__); \
        }                                                                                          \
    } while (0)

namespace kdk {
namespace internal {

void HandleAssert(const char* expr, const char* filename, u32 lineno, const char* function);
void HandleAssertf(const char* expr,
                   const char* filename,
                   u32 lineno,
                   const char* function,
                   const char* fmt,
                   ...);

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

#define DEFER_PRIVATE_JOIN(A, B) DEFER_PRIVATE_JOIN_INNER(A, B)
#define DEFER_PRIVATE_JOIN_INNER(A, B) A##B

#define DEFER                                                \
    const auto DEFER_PRIVATE_JOIN(__defer_guard, __LINE__) = \
        ::kdk::internal::DeferSyntaxSupport() + [&]()

}  // namespace internal
}  // namespace kdk
