#include <kandinsky/systems/system.h>

#include <kandinsky/platform.h>
#include <kandinsky/systems/system_manager.h>

namespace kdk {

constexpr String ToString(ESystemType system_type) {
#define X(ENUM_NAME, ...) \
    case ESystemType ::ENUM_NAME: return std::string_view(#ENUM_NAME);

    switch (system_type) {
        SYSTEM_TYPES(X)
        case ESystemType ::Invalid: return "<invalid>"sv;
        case ESystemType ::COUNT: return "<invalid>"sv;
    }
#undef X

    return "<invalid>"sv;
}

#define X(ENUM_NAME, STRUCT_NAME, ...)                                                       \
    PlatformState* STRUCT_NAME::GetPlatformState() { return _PlatformState; }                \
    EntityManager* STRUCT_NAME::GetEntityManager() { return _PlatformState->EntityManager; } \
    SystemManager* STRUCT_NAME::GetSystemManager() { return &_PlatformState->Systems; }      \
    GameMode* STRUCT_NAME::GetGameMode() { return &_PlatformState->GameMode; }
SYSTEM_TYPES(X)
#undef X

void* GetSystemOpaque(SystemManager* sm, ESystemType system_type) {
#define X(ENUM_NAME, STRUCT_NAME, ...)  \
    case ESystemType::ENUM_NAME: {      \
        return &sm->System_##ENUM_NAME; \
    }

    switch (system_type) {
        SYSTEM_TYPES(X)
        case ESystemType::Invalid: ASSERT(false); return nullptr;
        case ESystemType::COUNT: ASSERT(false); return nullptr;
    }
#undef X

    ASSERT(false);
    return nullptr;
}

}  // namespace kdk
