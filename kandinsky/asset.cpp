#include <kandinsky/asset.h>

#include <kandinsky/core/algorithm.h>
#include <kandinsky/core/serde.h>
#include <kandinsky/imgui.h>
#include <kandinsky/platform.h>

#include <charconv>
#include <ranges>

namespace kdk {

namespace asset_private {

String SerializeAssetToString(Arena* arena, AssetRegistry* assets, AssetHandle asset_handle) {
    auto [asset, _] = FindAssetOpaque(assets, asset_handle);
    ASSERT(asset);

    return Printf(arena,
                  "%s:%s:%d:%s",
                  ToString(asset->GetAssetType()).Str(),
                  asset->AssetOptions.IsBaseAsset ? "Base" : "External",
                  asset->Handle.AssetID,
                  asset->AssetPath.Str());
}

AssetHandle DeserializeAssetFromString(AssetRegistry* assets, String serialized_string) {
    auto parts =
        serialized_string.ToSV() | std::views::split(':') | std::views::transform([](auto&& range) {
            return std::string_view{range.begin(), range.end()};
        });

    FixedVector<std::string_view, 4> tokens;
    tokens.Push(parts.begin(), parts.end());
    if (tokens.Size != 4) {
        ASSERT(false);
        return {};
    }

    EAssetType serialized_asset_type = AssetTypeFromString(tokens[0]);
    if (serialized_asset_type == EAssetType::Invalid) {
        ASSERT(false);
        return {};
    }

    bool is_base_asset = false;
    if (tokens[1] == "Base"sv) {
        is_base_asset = true;
    } else if (tokens[1] != "External"sv) {
        ASSERT(false);
        return {};
    }

    i32 serialized_asset_id = NONE;
    if (auto [ptr, ec] = std::from_chars(tokens[2].data(),
                                         tokens[2].data() + tokens[2].size(),
                                         serialized_asset_id);
        ec != std::errc{}) {
        ASSERT(false);
        return {};
    }

    String serialized_asset_path(tokens[3]);

    if (is_base_asset) {
        return FindAssetHandle(assets, serialized_asset_type, serialized_asset_path);
    }

    AssetHandle result =
        DeserializeAssetFromDisk(assets, serialized_asset_type, serialized_asset_path);
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

AssetHandle AssetHandle::Build(EAssetType asset_type, i32 asset_id, i32 index) {
    return AssetHandle{
        ._Value = ((i32)asset_type << 24) | (index & 0xFFFFFF),
        .AssetID = asset_id,
    };
}

EAssetType AssetHandle::GetAssetType() const {
    if (_Value == NONE) {
        return EAssetType::Invalid;
    }
    EAssetType type = static_cast<EAssetType>((u8)(_Value >> 24));
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

// IMGUI -------------------------------------------------------------------------------------------

void ImGui_AssetHandleOpaque(AssetRegistry* registry,
                             String label,
                             EAssetType asset_type,
                             void* input_handle) {
    if (asset_type != EAssetType::Texture) {
        ImGui::Text("%s: Unsupported asset type (%s)", label.Str(), ToString(asset_type).Str());
        return;
    }

    TextureAssetHandle* handle = (TextureAssetHandle*)input_handle;

    float width_before = ImGui::GetContentRegionAvail().x;
    ImGui::Text("%s: ", label.Str());
    ImGui::SameLine();
    float width_after = ImGui::GetContentRegionAvail().x;

    static ImGuiTextFilter filter;
    float width = width_before - width_after;
    filter.Draw("Filter", ImGui::CalcItemWidth() - width);

    FixedVector<Texture*, 64> textures;
    {
        auto listed = registry->TextureHolder.ListAssets();
        for (Texture& texture : listed) {
            if (!IsValid(texture)) {
                continue;
            }

            if (!filter.PassFilter(texture.GetAsset().AssetPath.Str())) {
                continue;
            }

            textures.Push(&texture);
        }
    }

    textures.SortPred([](const auto& lhs, const auto& rhs) {
        return lhs->GetAsset().AssetPath < rhs->GetAsset().AssetPath;
    });

    i32 selected_index = NONE;
    Texture* selected_texture = nullptr;

    FixedVector<const char*, 64> texture_names;
    for (i32 i = 0; i < textures.Size; i++) {
        Texture* texture = textures[i];
        if (handle->AssetID == texture->GetAsset().Handle.AssetID) {
            selected_index = i;
            selected_texture = texture;
        }
        texture_names.Push(texture->GetAsset().AssetPath.Str());
    }

    const char* preview_name = nullptr;
    if (selected_index >= 0 && selected_index < textures.Size) {
        selected_texture = textures[selected_index];
        preview_name = texture_names[selected_index];
    }

    bool changed_selection = false;
    if (ImGui::BeginCombo("##SelectTexture", preview_name)) {
        for (i32 i = 0; i < textures.Size; i++) {
            ImGui::PushID(i);

            Texture* texture = textures[i];
            bool is_selected = (selected_texture == texture);

            static constexpr i32 kRowSize = 36;

            if (ImGui::Selectable("", is_selected, 0, ImVec2(0, kRowSize))) {
                changed_selection = true;
                selected_texture = texture;
                selected_index = i;
            }

            // Draw image and text on the same line as the selectable
            // We need to go back up to draw over the selectable
            ImVec2 backup_pos = ImGui::GetCursorPos();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - kRowSize);  // Go back up

            ImGui::Image((ImTextureID)texture->Handle, ImVec2(32, 32));
            ImGui::SameLine();
            ImGui::Text("%s", texture_names[i]);

            // Restore cursor position
            ImGui::SetCursorPos(backup_pos);

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }

            ImGui::PopID();
        }

        ImGui::EndCombo();
    }

    if (selected_texture && IsValid(*selected_texture)) {
        if (ImGui::IsItemHovered()) {
            if (ImGui::BeginTooltip()) {
                ImGui::Text("%s", selected_texture->GetAsset().AssetPath.Str());
                float _width = Min((float)selected_texture->Width, 256.0f);
                float _height = Min((float)selected_texture->Height, 256.0f);
                ImGui::Image((ImTextureID)selected_texture->Handle, ImVec2(_width, _height));
                ImGui::EndTooltip();
            }
        }
    }

    if (changed_selection) {
        Reset(handle, selected_texture->GetAsset().Handle);
    }
}

}  // namespace kdk
