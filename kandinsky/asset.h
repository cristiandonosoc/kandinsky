#pragma once

#include <kandinsky/core/string.h>

namespace kdk {

struct Asset;
struct AssetRegistry;
struct SerdeArchive;

// X macro for defining entity types.
// Format: (enum_name, struct_name, max_count)
#define ASSET_TYPES(X)          \
    X(Mesh, Mesh, 1024)         \
    X(Model, Model, 128)        \
    X(Texture, Texture, 128)    \
    X(Material, Material, 1024) \
    X(Shader, Shader, 64)

#define X(enum_name, ...) enum_name,
enum class EAssetType : u8 {
    Invalid = 0,
    ASSET_TYPES(X) COUNT,
};
#undef X

String ToString(EAssetType type);
EAssetType AssetTypeFromString(String string);

struct AssetOptions {
    bool IsBaseAsset : 1 = false;
};

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
    i32 Value = NONE;
    i32 AssetID = NONE;

    EAssetType GetAssetType() const;
    i32 GetIndex() const { return Value & 0xFFFFFF; }

    static AssetHandle Build(EAssetType asset_type, i32 asset_id, i32 index);
    static AssetHandle Build(const Asset& asset, i32 index);
};
inline bool IsValid(const AssetHandle& handle) { return handle.Value != NONE; }
void Serialize(SerdeArchive* sa, AssetHandle* handle);
void SerializeAssetHandle(SerdeArchive* sa, EAssetType type, AssetHandle* handle);

// Create typed asset handles (eg. MeshAssetHandle, ModelAssetHandle, etc.)
//
// It has no AssetHandle constructor in order to avoid permitting neighbour handles
// (eg. ModelAssetHandle and TextureAssetHandle) to be converted into one another.
// A bit annoying to have to wrap the handlers in {} for construction, but detects common errors.
#define X(enum_name, ...)                                                           \
    struct enum_name##AssetHandle : public AssetHandle {                            \
        static constexpr EAssetType kAssetType = EAssetType::enum_name;             \
    };                                                                              \
    inline void Serialize(SerdeArchive* sa, enum_name##AssetHandle* asset_handle) { \
        Serialize(sa, (AssetHandle*)asset_handle);                                  \
    }

ASSET_TYPES(X)
#undef X

}  // namespace kdk
