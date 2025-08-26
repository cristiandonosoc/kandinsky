#pragma once

#include <kandinsky/core/string.h>

namespace kdk {

struct Asset;
struct AssetRegistry;
struct SerdeArchive;

// X macro for defining entity types.
// Format: (enum_name, struct_name, max_count)
#define ASSET_TYPES(X)       \
    X(Mesh, Mesh, 1024)      \
    X(Model, Model, 128)     \
    X(Texture, Texture, 128) \
    X(Material, Material, 1024)

#define X(enum_name, ...) enum_name,
enum class EAssetType : u8 {
    Invalid = 0,
    ASSET_TYPES(X) COUNT,
};
#undef X

String ToString(EAssetType type);
EAssetType AssetTypeFromString(String string);

#define GENERATE_ASSET(name) static constexpr EAssetType kAssetType = EAssetType::name;

struct AssetHandle {
    // 8-bit: type, 24-bit: index.
    i32 Value = NONE;
    i32 AssetID = NONE;

    static AssetHandle Build(const Asset& asset, i32 index);
    EAssetType GetAssetType() const;
    i32 GetIndex() const { return Value & 0xFFFFFF; }
};
inline bool IsValid(const AssetHandle& handle) { return handle.Value != NONE; }
void Serialize(SerdeArchive* sa, AssetHandle* handle);
void SerializeAssetHandle(SerdeArchive* sa, EAssetType type, AssetHandle* handle);

// Create typed asset handles (eg. MeshAssetHandle, ModelAssetHandle, etc.)
#define X(enum_name, ...)                                                   \
    struct enum_name##AssetHandle : public AssetHandle {                    \
        static constexpr EAssetType kAssetType = EAssetType::enum_name;     \
        enum_name##AssetHandle() = default;                                 \
        enum_name##AssetHandle(AssetHandle handle) : AssetHandle(handle) {} \
    };
ASSET_TYPES(X)
#undef X

}  // namespace kdk
