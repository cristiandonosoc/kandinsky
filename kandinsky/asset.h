#pragma once

#include <kandinsky/core/string.h>

namespace kdk {

struct SerdeArchive;

enum class EAssetType : u8 {
    Invalid,
    Model,
    Shader,
    Texture,
    COUNT
};

#define GENERATE_ASSET(name) static constexpr EAssetType kAssetType = EAssetType::name;

struct Asset {
    i32 ID = NONE;
    EAssetType Type = EAssetType::Invalid;
    FixedString<128> AssetPath;
    void* UnderlyingAsset = nullptr;
};

struct AssetHandle {
    i32 Value = NONE;
};
inline bool IsValid(const AssetHandle& handle) { return handle.Value != NONE; }
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
