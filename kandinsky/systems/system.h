#pragma once

#include <kandinsky/core/defines.h>
#include <kandinsky/core/string.h>

namespace kdk {

struct SystemManager;

// Format: (ENUM_NAME, STRUCT_NAME)
#define SYSTEM_TYPES(X) X(Schedule, ScheduleSystem)

#define X(ENUM_NAME, ...) ENUM_NAME,
enum class ESystemType : u8 {
    Invalid = 0,
    SYSTEM_TYPES(X) COUNT,
};
#undef X
constexpr String ToString(ESystemType system_type);

#define GENERATE_SYSTEM(ENUM_NAME)                                     \
    static constexpr ESystemType kSystemType = ESystemType::ENUM_NAME; \
    static constexpr String kSystemName{std::string_view(#ENUM_NAME)};

void* GetSystemOpaque(SystemManager* sm, ESystemType system_type);

template <typename T>
T* GetSystem(SystemManager* sm) {
    return (T*)GetSystemOpaque(sm, T::kSystemType);
}

}  // namespace kdk
