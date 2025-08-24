#include <kandinsky/asset.h>

#include <kandinsky/core/serde.h>
#include <kandinsky/platform.h>

#include <charconv>
#include <ranges>

namespace kdk {

namespace asset_private {

String SerializeAssetToString(Arena* arena, const Asset& asset) {
    return Printf(arena,
                  "%s:%d:%s",
                  ToString(asset.Type).Str(),
                  asset.AssetID,
                  asset.AssetPath.ToString());
}

std::pair<EAssetType, String> DeserializeAssetFromString(String serialized_string) {
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

    // Ensure the asset id is the same.
    if (serialized_asset_id != GenerateAssetID(serialized_asset_type, tokens[2])) {
        ASSERT(false);
        return {};
    }

    return {serialized_asset_type, tokens[2]};
}

}  // namespace asset_private

String ToString(EAssetType type) {
#define X(enum_name, ...) \
    case EAssetType::enum_name: return "enum_name"sv;

    switch (type) {
        case EAssetType::Invalid: return "<invalid>"sv; ASSET_TYPES(X)
        case EAssetType::COUNT: return "<count>"sv;
    }
#undef X
    ASSERT(false);
    return "<unknown>"sv;
}

EAssetType FromString(String string) {
#define X(enum_name, ...)               \
    if (string.Equals("enum_name"sv)) { \
        return EAssetType::enum_name;   \
    }

    ASSET_TYPES(X)

#undef X

    ASSERT(false);
    return EAssetType::Invalid;
}

i32 GenerateAssetID(EAssetType type, String asset_path) {
    return IDFromString(asset_path.Str()) + (i32)type;
}

AssetHandle AssetHandle::Build(Asset* asset, i32 index) {
    return AssetHandle{
        .Value = ((i32)asset->Type << 24) | (index & 0xFFFFFF),
        .AssetID = asset->AssetID,
    };
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
    // AssetRegistry* asset_registry = sa->SerdeContext->AssetRegistry;
    // ASSERT(asset_registry);

    // if (sa->Mode == ESerdeMode::Serialize) {
    // 	if (Asset* asset = FindAsset(asset_registry, *handle)) {
    // 		i32 value = handle->Value;
    // 		Serialize(sa, &value);
    // 		return;
    // 	}
    // 	else {

    // 	}

    // 	Serialize(sa, &value);
    // 	return;
    // }

    (void)sa;
    (void)handle;
    ASSERTF(false, "Not implemented yet");
}

void SerializeAssetHandle(SerdeArchive* sa, EAssetType type, AssetHandle* handle) {
    (void)sa;
    (void)type;
    (void)handle;
    ASSERTF(false, "Not implemented yet");
}

}  // namespace kdk
