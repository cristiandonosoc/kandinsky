#include <kandinsky/asset.h>

#include <kandinsky/core/serde.h>
#include <kandinsky/platform.h>

#include <charconv>
#include <ranges>

namespace kdk {

namespace asset_private {

String SerializeAssetToString(Arena* arena, AssetRegistry* assets, AssetHandle asset_handle) {
    auto [asset, _] = FindAsset(assets, asset_handle);
    ASSERT(asset);

    return Printf(arena,
                  "%s:%d:%s",
                  ToString(asset->Type).Str(),
                  asset->AssetID,
                  asset->AssetPath.Str());
}

AssetHandle DeserializeAssetFromString(AssetRegistry* assets, String serialized_string) {
    auto parts =
        serialized_string.ToSV() | std::views::split(':') | std::views::transform([](auto&& range) {
            return std::string_view{range.begin(), range.end()};
        });

    FixedArray<std::string_view, 4> tokens;
    tokens.Push(parts.begin(), parts.end());
    if (tokens.Size != 3) {
        ASSERT(false);
        return {};
    }

    EAssetType serialized_asset_type = AssetTypeFromString(tokens[0]);
    if (serialized_asset_type == EAssetType::Invalid) {
        ASSERT(false);
        return {};
    }

    i32 serialized_asset_id = NONE;
    if (auto [ptr, ec] = std::from_chars(tokens[1].data(),
                                         tokens[1].data() + tokens[1].size(),
                                         serialized_asset_id);
        ec != std::errc{}) {
        ASSERT(false);
        return {};
    }

    String serialized_asset_path(tokens[2]);

    AssetHandle result = DeserializeAssetFromDisk(assets, serialized_asset_type, tokens[2]);
    ASSERT(IsValid(result));

    return result;
}

}  // namespace asset_private

String ToString(EAssetType type) {
#define X(enum_name, ...) \
    case EAssetType::enum_name: return String(#enum_name);

    switch (type) {
        case EAssetType::Invalid: return "<invalid>"sv; ASSET_TYPES(X)
        case EAssetType::COUNT: return "<count>"sv;
    }
#undef X
    ASSERT(false);
    return "<unknown>"sv;
}

EAssetType AssetTypeFromString(String string) {
#define X(enum_name, ...)                              \
    if (string.Equals(std::string_view(#enum_name))) { \
        return EAssetType::enum_name;                  \
    }

    ASSET_TYPES(X)

#undef X

    ASSERT(false);
    return EAssetType::Invalid;
}

i32 GenerateAssetID(EAssetType type, String asset_path) {
    return IDFromString(asset_path.Str()) + (i32)type;
}

AssetHandle AssetHandle::Build(EAssetType asset_type, i32 asset_id, i32 index) {
    return AssetHandle{
        .Value = ((i32)asset_type << 24) | (index & 0xFFFFFF),
        .AssetID = asset_id,
    };
}

AssetHandle AssetHandle::Build(const Asset& asset, i32 index) {
    return AssetHandle::Build(asset.Type, asset.AssetID, index);
}

EAssetType AssetHandle::GetAssetType() const {
    if (Value == NONE) {
        return EAssetType::Invalid;
    }
    EAssetType type = static_cast<EAssetType>((u8)(Value >> 24));
    ASSERTF(type > EAssetType::Invalid && type < EAssetType::COUNT,
            "Invalid asset type: %d",
            (u8)type);
    return type;
}

void Serialize(SerdeArchive* sa, AssetHandle* handle) {
    using namespace asset_private;

    AssetRegistry* assets = sa->SerdeContext->AssetRegistry;
    auto scoped_arena = sa->TempArena->GetScopedArena();

    if (sa->Mode == ESerdeMode::Serialize) {
        String serialized = SerializeAssetToString(scoped_arena, assets, *handle);
        Serde(sa, "Asset", &serialized);
    } else {
        String serialized;
        Serde(sa, "Asset", &serialized);
        if (serialized.IsEmpty()) {
            *handle = {};
        } else {
            *handle = DeserializeAssetFromString(assets, serialized);
        }
    }
}

}  // namespace kdk
