#pragma once

#include <kandinsky/core/string.h>

namespace kdk {

struct Asset;
struct AssetRegistry;
struct SerdeArchive;

// X macro for defining entity types.
// Format: (ENUM_NAME, STRUCT_NAME, MAX_COUNT)
#define ASSET_TYPES(X)          \
    X(Mesh, Mesh, 1024)         \
    X(Model, Model, 128)        \
    X(Texture, Texture, 128)    \
    X(Material, Material, 1024) \
    X(Shader, Shader, 64)       \
    X(Font, Font, 16)

// Define the enum.
#define X(ENUM_NAME, ...) ENUM_NAME,
enum class EAssetType : u8 {
    Invalid = 0,
    ASSET_TYPES(X) COUNT,
};
#undef X

// Forward declare the types.
#define X(ENUM_NAME, STRUCT_NAME, ...) struct STRUCT_NAME;
ASSET_TYPES(X)
#undef X

String ToString(EAssetType type);
EAssetType AssetTypeFromString(String string);

struct AssetOptions {
    bool IsBaseAsset : 1 = false;
    bool IsIcon : 1 = false;
};
static_assert(sizeof(AssetOptions) == 1);

#define GENERATE_ASSET(name)                                   \
    static constexpr EAssetType kAssetType = EAssetType::name; \
                                                               \
    Asset* _AssetPtr = nullptr;                                \
    const Asset& GetAsset() const { return *_AssetPtr; }

#define GENERATE_ASSET_PARAMS()                                                   \
    static constexpr bool kCreateAssetStructRequiresGENERATE_ASSET_PARAMS = true; \
    AssetOptions AssetOptions = {};

struct AssetHandle {
    // 8-bit: type, 24-bit: index.
    i32 _Value = NONE;
    i32 AssetID = NONE;

    EAssetType GetAssetType() const;
    i32 GetIndex() const { return _Value & 0xFFFFFF; }

    bool operator==(const AssetHandle& other) const = default;

    static AssetHandle Build(EAssetType asset_type, i32 asset_id, i32 index);
};
inline bool IsValid(const AssetHandle& handle) { return handle._Value != NONE; }
inline void Reset(AssetHandle* handle, const AssetHandle& other) { *handle = other; }
void Serialize(SerdeArchive* sa, AssetHandle* handle);
void SerializeAssetHandle(SerdeArchive* sa, EAssetType type, AssetHandle* handle);

// Create typed asset handles (eg. MeshAssetHandle, ModelAssetHandle, etc.)
//
// It has no AssetHandle constructor in order to avoid permitting neighbour handles
// (eg. ModelAssetHandle and TextureAssetHandle) to be converted into one another.
// A bit annoying to have to wrap the handlers in {} for construction, but detects common errors.
#define X(ENUM_NAME, ...)                                                           \
    struct ENUM_NAME##AssetHandle : public AssetHandle {                            \
        static constexpr EAssetType kAssetType = EAssetType::ENUM_NAME;             \
    };                                                                              \
    inline void Serialize(SerdeArchive* sa, ENUM_NAME##AssetHandle* asset_handle) { \
        Serialize(sa, (AssetHandle*)asset_handle);                                  \
    }

ASSET_TYPES(X)
#undef X

AssetHandle FindAssetHandle(AssetRegistry* assets, EAssetType asset_type, String asset_path);
std::pair<const Asset*, void*> FindAssetOpaque(AssetRegistry* assets, AssetHandle handle);

// Generate getter for the handles.
#define X(ENUM_NAME, STRUCT_NAME, ...)                                                   \
    inline ENUM_NAME##AssetHandle Find##ENUM_NAME##Handle(AssetRegistry* assets,         \
                                                          String asset_path) {           \
        AssetHandle handle = FindAssetHandle(assets, EAssetType::ENUM_NAME, asset_path); \
        return {handle};                                                                 \
    }                                                                                    \
                                                                                         \
    inline STRUCT_NAME* Find##ENUM_NAME##Asset(AssetRegistry* assets,                    \
                                               ENUM_NAME##AssetHandle handle) {          \
        auto [_, opaque] = FindAssetOpaque(assets, handle);                              \
        return (STRUCT_NAME*)opaque;                                                     \
    }

ASSET_TYPES(X)
#undef X

// IMGUI -------------------------------------------------------------------------------------------

void ImGui_AssetHandleOpaque(AssetRegistry* registry,
                             String label,
                             EAssetType asset_type,
                             void* opaque);

#define X(ENUM_NAME, ...)                                                        \
    inline void ImGui_##ENUM_NAME##AssetHandle(AssetRegistry* registry,          \
                                               String label,                     \
                                               ENUM_NAME##AssetHandle* handle) { \
        ImGui_AssetHandleOpaque(registry, label, EAssetType::ENUM_NAME, handle); \
    }
ASSET_TYPES(X)
#undef X

}  // namespace kdk
