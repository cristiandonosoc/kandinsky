#include <kandinsky/systems/system_manager.h>

namespace kdk {

namespace system_manager_internal {

template <typename T>
constexpr bool HasSystemInitV = requires(PlatformState* ps, T* ptr) { ::kdk::Init(ps, ptr); };

template <typename T>
bool Init_Internal(PlatformState* ps, T* system) {
    return ::kdk::Init(ps, system);
}

}  // namespace system_manager_internal

bool InitSystems(PlatformState* ps, SystemManager* sm) {
    using namespace system_manager_internal;

#define X(ENUM_NAME, STRUCT_NAME, ...)                 \
    case ESystemType::ENUM_NAME: {                     \
        STRUCT_NAME* system = &sm->System_##ENUM_NAME; \
        system->_PlatformState = ps;                   \
        if constexpr (HasSystemInitV<STRUCT_NAME>) {   \
            if (!Init_Internal(ps, system)) {          \
                return false;                          \
            }                                          \
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

namespace system_manager_internal {

template <typename T>
constexpr bool HasSystemShutdownV = requires(T* ptr) { ::kdk::Shutdown(ptr); };

template <typename T>
void Shutdown_Internal(T* system) {
    ::kdk::Shutdown(system);
}

}  // namespace system_manager_internal

void ShutdownSystems(SystemManager* sm) {
    using namespace system_manager_internal;

#define X(ENUM_NAME, STRUCT_NAME, ...)                   \
    case ESystemType::ENUM_NAME: {                       \
        STRUCT_NAME* system = &sm->System_##ENUM_NAME;   \
        if constexpr (HasSystemShutdownV<STRUCT_NAME>) { \
            Shutdown_Internal(system);                   \
        }                                                \
        system->_PlatformState = nullptr;                \
        break;                                           \
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

namespace system_manager_internal {

template <typename T>
constexpr bool HasSystemStartV = requires(T* ptr) { ::kdk::Start(ptr); };

// We require this template because all arms of a constexpr evaluation are evaluated in compile
// time, even if they evaluate to false. Meaning that I cannot call functions that would not exist,
// though I'm sure I use this trick somewhere else, so it might be that `EntityHasUpdateV` behaves
// incorrectly with constexpr.
//
// Having this function permits me to workaround this and get the thing to compile and only call
// update if it is defined for the entity.
template <typename T>
void Start_Internal(T* system) {
    ::kdk::Start(system);
}

}  // namespace system_manager_internal

void StartSystems(SystemManager* sm) {
    using namespace system_manager_internal;

#define X(ENUM_NAME, STRUCT_NAME, ...)                 \
    case ESystemType::ENUM_NAME: {                     \
        STRUCT_NAME* system = &sm->System_##ENUM_NAME; \
        if constexpr (HasSystemStartV<STRUCT_NAME>) {  \
            Start_Internal(system);                    \
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

namespace system_manager_internal {

template <typename T>
static constexpr bool HasSystemStopV = requires(T* ptr) { ::kdk::Stop(ptr); };

// We require this template because all arms of a constexpr evaluation are evaluated in compile
// time, even if they evaluate to false. Meaning that I cannot call functions that would not exist,
// though I'm sure I use this trick somewhere else, so it might be that `EntityHasUpdateV` behaves
// incorrectly with constexpr.
//
// Having this function permits me to workaround this and get the thing to compile and only call
// update if it is defined for the entity.
template <typename T>
void Stop_Internal(T* system) {
    Stop(system);
}

}  // namespace system_manager_internal

void StopSystems(SystemManager* sm) {
    using namespace system_manager_internal;

#define X(ENUM_NAME, STRUCT_NAME, ...)                 \
    case ESystemType::ENUM_NAME: {                     \
        STRUCT_NAME* system = &sm->System_##ENUM_NAME; \
        if constexpr (HasSystemStopV<STRUCT_NAME>) {   \
            Stop_Internal(system);                     \
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

namespace system_manager_internal {

template <typename T>
static constexpr bool HasSystemUpdateV = requires(T* ptr, float dt) { ::kdk::Update(ptr, dt); };

// We require this template because all arms of a constexpr evaluation are evaluated in compile
// time, even if they evaluate to false. Meaning that I cannot call functions that would not exist,
// though I'm sure I use this trick somewhere else, so it might be that `EntityHasUpdateV` behaves
// incorrectly with constexpr.
//
// Having this function permits me to workaround this and get the thing to compile and only call
// update if it is defined for the entity.
template <typename T>
void Update_Internal(T* system, float dt) {
    Update(system, dt);
}

}  // namespace system_manager_internal

void UpdateSystems(SystemManager* sm, float dt) {
    using namespace system_manager_internal;

#define X(ENUM_NAME, STRUCT_NAME, ...)                 \
    case ESystemType::ENUM_NAME: {                     \
        STRUCT_NAME* system = &sm->System_##ENUM_NAME; \
        if constexpr (HasSystemUpdateV<STRUCT_NAME>) { \
            Update_Internal(system, dt);               \
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

}  // namespace kdk
