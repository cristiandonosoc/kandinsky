#include <kandinsky/systems/system_manager.h>

namespace kdk {

bool InitSystems(PlatformState* ps, SystemManager* sm) {
#define X(ENUM_NAME, STRUCT_NAME, ...)                 \
    case ESystemType::ENUM_NAME: {                     \
        STRUCT_NAME* system = &sm->System_##ENUM_NAME; \
        if (!Init(ps, system)) {                       \
            return false;                              \
        }                                              \
        break;                                         \
    }

    for (i32 i = (i32)(ESystemType::Invalid) + 1; i < (i32)ESystemType::COUNT; i++) {
        ESystemType system_type = (ESystemType)i;
        switch (system_type) {
            SYSTEM_TYPES(X)
            case ESystemType::Invalid: ASSERT(false); break;
            case ESystemType::COUNT: ASSERT(false); break;
        }
    }

#undef X

    return true;
}

void ShutdownSystems(SystemManager* sm) {
#define X(ENUM_NAME, STRUCT_NAME, ...)                 \
    case ESystemType::ENUM_NAME: {                     \
        STRUCT_NAME* system = &sm->System_##ENUM_NAME; \
        Shutdown(system);                              \
        break;                                         \
    }

    // We iterate going down.
    for (i32 i = (i32)(ESystemType::COUNT)-1; i > (i32)ESystemType::Invalid; i--) {
        ESystemType system_type = (ESystemType)i;
        switch (system_type) {
            SYSTEM_TYPES(X)
            case ESystemType::Invalid: ASSERT(false); break;
            case ESystemType::COUNT: ASSERT(false); break;
        }
    }

#undef X
}

template <typename T>
static constexpr bool HasSystemStartV = requires(T* ptr) { ::kdk::Start(ptr); };

void StartSystems(SystemManager* sm) {
#define X(ENUM_NAME, STRUCT_NAME, ...)                 \
    case ESystemType::ENUM_NAME: {                     \
        STRUCT_NAME* system = &sm->System_##ENUM_NAME; \
        if constexpr (HasSystemStartV<STRUCT_NAME>) {  \
            Start(system);                             \
        }                                              \
        break;                                         \
    }

    for (i32 i = (i32)(ESystemType::Invalid) + 1; i < (i32)ESystemType::COUNT; i++) {
        ESystemType system_type = (ESystemType)i;
        switch (system_type) {
            SYSTEM_TYPES(X)
            case ESystemType::Invalid: ASSERT(false); break;
            case ESystemType::COUNT: ASSERT(false); break;
        }
    }

#undef X
}

template <typename T>
static constexpr bool HasSystemStopV = requires(T* ptr) { ::kdk::Stop(ptr); };

void StopSystems(SystemManager* sm) {
#define X(ENUM_NAME, STRUCT_NAME, ...)                 \
    case ESystemType::ENUM_NAME: {                     \
        STRUCT_NAME* system = &sm->System_##ENUM_NAME; \
        if constexpr (HasSystemStopV<STRUCT_NAME>) {   \
            Stop(system);                              \
        }                                              \
        break;                                         \
    }

    // We iterate going down.
    for (i32 i = (i32)(ESystemType::COUNT)-1; i > (i32)ESystemType::Invalid; i--) {
        ESystemType system_type = (ESystemType)i;
        switch (system_type) {
            SYSTEM_TYPES(X)
            case ESystemType::Invalid: ASSERT(false); break;
            case ESystemType::COUNT: ASSERT(false); break;
        }
    }

#undef X
}

void UpdateSystems(SystemManager* sm) {
#define X(ENUM_NAME, STRUCT_NAME, ...)                 \
    case ESystemType::ENUM_NAME: {                     \
        STRUCT_NAME* system = &sm->System_##ENUM_NAME; \
        Update(system);                                \
        break;                                         \
    }

    for (i32 i = (i32)(ESystemType::Invalid) + 1; i < (i32)ESystemType::COUNT; i++) {
        ESystemType system_type = (ESystemType)i;
        switch (system_type) {
            SYSTEM_TYPES(X)
            case ESystemType::Invalid: ASSERT(false); break;
            case ESystemType::COUNT: ASSERT(false); break;
        }
    }

#undef X
}

}  // namespace kdk
