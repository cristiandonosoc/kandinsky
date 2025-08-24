#pragma once

#include <kandinsky/core/string.h>

namespace kdk {

struct SerdeArchive;

// X macro for defining entity types.
// Format: (enum_name, struct_name, max_count)
#define ASSET_TYPES(X) X(Mesh, Mesh, 1024)

#define X(enum_name, ...) enum_name,
enum class EAssetType : u8 {
    Invalid = 0,
    ASSET_TYPES(X) COUNT,
};
#undef X

String ToString(EAssetType type);
EAssetType AssetTypeFromString(String string);

#define GENERATE_ASSET(name) static constexpr EAssetType kAssetType = EAssetType::name;

struct Asset {
    i32 AssetID = NONE;
    EAssetType Type = EAssetType::Invalid;
    FixedString<128> AssetPath;
    void* UnderlyingAsset = nullptr;
};

i32 GenerateAssetID(EAssetType type, String asset_path);

struct AssetDescriptor {
    EAssetType Type = EAssetType::Invalid;
    FixedString<128> AssetPath;
};

struct AssetHandle {
    // 8-bit: type, 24-bit: index.
    i32 Value = NONE;
    i32 AssetID = NONE;

    static AssetHandle Build(Asset* asset, i32 index);
    EAssetType GetAssetType() const;
    i32 GetIndex() const { return Value & 0xFFFFFF; }
};
AssetHandle CreateAssetHandle(Asset* asset, i32 index);

inline bool IsValid(const AssetHandle& handle) { return handle.Value != NONE; }
void Serialize(SerdeArchive* sa, AssetHandle* handle);
void SerializeAssetHandle(SerdeArchive* sa, EAssetType type, AssetHandle* handle);

template <typename T>
struct AssetHandleT : public AssetHandle {
    static constexpr EAssetType kAssetType = T::kAssetType;
};

template <typename T>
void Serialize(SerdeArchive* sa, AssetHandleT<T>* handle) {
    SerializeAssetHandle(sa, T::kAssetType, handle);
}

}  // namespace kdk
