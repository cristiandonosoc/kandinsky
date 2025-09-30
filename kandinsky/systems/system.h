#pragma once

#include <kandinsky/core/defines.h>
#include <kandinsky/core/string.h>

namespace kdk {

struct PlatformState;
struct EntityManager;
struct SystemManager;

// Format: (ENUM_NAME, STRUCT_NAME)
#define SYSTEM_TYPES(X)         \
    X(Schedule, ScheduleSystem) \
    X(Enemy, EnemySystem)

#define X(ENUM_NAME, ...) ENUM_NAME,
enum class ESystemType : u8 {
    Invalid = 0,
    SYSTEM_TYPES(X) COUNT,
};
#undef X
constexpr String ToString(ESystemType system_type);

#define GENERATE_SYSTEM(ENUM_NAME)                                     \
    static constexpr ESystemType kSystemType = ESystemType::ENUM_NAME; \
    static constexpr String kSystemName{std::string_view(#ENUM_NAME)}; \
    PlatformState* _PlatformState = nullptr;                           \
    PlatformState* GetPlatformState();                                 \
    EntityManager* GetEntityManager();                                 \
    SystemManager* GetSystemManager();

void* GetSystemOpaque(SystemManager* sm, ESystemType system_type);

template <typename T>
T* GetSystem(SystemManager* sm) {
    return (T*)GetSystemOpaque(sm, T::kSystemType);
}

}  // namespace kdk
