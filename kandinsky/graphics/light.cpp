#include <kandinsky/graphics/light.h>

#include <kandinsky/core/math.h>
#include <kandinsky/core/serde.h>
#include <kandinsky/debug.h>
#include <kandinsky/entity.h>
#include <kandinsky/graphics/model.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/graphics/render_state.h>

#include <imgui.h>

namespace kdk {

const char* ToString(ELightType v) {
    switch (v) {
        case ELightType::Invalid: return "<INVALID>"; break;
        case ELightType::Point: return "Point";
        case ELightType::Directional: return "Directional";
        case ELightType::Spotlight: return "Spotlight";
        case ELightType::COUNT: return "<COUNT>"; break;
    }

    assert(false);
    return "<UNKNOWN>";
}

void BuildImGui(LightColor* light_color) {
    ImGui::ColorEdit3("Ambient", GetPtr(light_color->Ambient), ImGuiColorEditFlags_Float);
    ImGui::ColorEdit3("Diffuse", GetPtr(light_color->Diffuse), ImGuiColorEditFlags_Float);
    ImGui::ColorEdit3("Specular", GetPtr(light_color->Specular), ImGuiColorEditFlags_Float);
}

void Serialize(SerdeArchive* sa, LightColor* lc) {
    SERDE(sa, lc, Ambient);
    SERDE(sa, lc, Diffuse);
    SERDE(sa, lc, Specular);
}

// PointLight --------------------------------------------------------------------------------------

void BuildImGui(PointLightComponent* pl) {
    Entity* owner = pl->GetOwner();
    ASSERT(owner);

    ImGui::InputFloat3("Position", GetPtr(owner->Transform.Position));
    BuildImGui(&pl->Color);
    ImGui::DragFloat("MinRadius", &pl->MinRadius, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat("MaxRadius", &pl->MaxRadius, 0.01f, 0.01f, 10.0f);

    ImGui::Text("Attenuation");

    ImGui::DragFloat("Constant", &pl->AttenuationConstant, 0.001f, 0.00f, 1.0f);
    ImGui::DragFloat("Linear", &pl->AttenuationLinear, 0.001f, 0.00f, 1.0f);
    ImGui::DragFloat("Quadratic", &pl->AttenuationQuadratic, 0.001f, 0.00f, 1.0f);
}

void BuildGizmos(PlatformState* ps, PointLightComponent* pl) {
    Entity* owner = pl->GetOwner();
    ASSERT(owner);

    Transform& transform = owner->Transform;
    Debug::DrawSphere(ps, transform.Position, pl->MinRadius, 16, Color32::Black);
    Debug::DrawSphere(ps, transform.Position, pl->MaxRadius, 16, Color32::Grey);
}

void Serialize(SerdeArchive* sa, PointLightComponent* pl) {
    SERDE(sa, pl, Color);
    SERDE(sa, pl, MinRadius);
    SERDE(sa, pl, MaxRadius);
    SERDE(sa, pl, AttenuationConstant);
    SERDE(sa, pl, AttenuationLinear);
    SERDE(sa, pl, AttenuationQuadratic);
    // Note: RS_ViewPosition is a render state cache, no need to serialize it
}

// void Draw(const PointLightComponent& pl,
//           const Shader& shader,
//           const Mesh& mesh,
//           const RenderState& rs) {
//     const Entity* owner = pl.GetOwner();
//     ASSERT(owner);
//
//     Use(shader);
//
//     Mat4 model(1.0f);
//     model = Translate(model, Vec3(owner->Transform.Position));
//     model = Scale(model, Vec3(0.2f));
//
//     SetMat4(shader, "uModel", GetPtr(model));
//     SetMat4(shader, "uViewProj", GetPtr(rs.M_ViewProj));
//
//     Draw(mesh, shader, rs);
// }

void BuildImGui(DirectionalLightComponent* dl) {
    ImGui::InputFloat3("Direction", GetPtr(dl->Direction));
    BuildImGui(&dl->Color);
}

void Serialize(SerdeArchive* sa, DirectionalLightComponent* dl) {
    SERDE(sa, dl, Direction);
    SERDE(sa, dl, Color);
    // Note: RS_ViewDirection is a render state cache, no need to serialize it
}

// Spotlight ---------------------------------------------------------------------------------------

void Recalculate(SpotlightComponent* sl) {
    const Entity* owner = sl->GetOwner();
    ASSERT(owner);

    sl->MaxCutoffDistance = Distance(owner->Transform.Position, sl->Target);
    sl->MinCutoffDistance = sl->MaxCutoffDistance * 0.9f;
    sl->InnerRadiusDeg = sl->OuterRadiusDeg * 0.9f;
}

void BuildImGui(SpotlightComponent* sl) {
    Entity* owner = sl->GetOwner();
    ASSERT(owner);

    bool recalculate = false;
    recalculate &= ImGui::InputFloat3("Position", GetPtr(owner->Transform.Position));
    recalculate &= ImGui::InputFloat3("Target", GetPtr(sl->Target));

    ImGui::InputFloat("Length", &sl->MaxCutoffDistance, ImGuiInputTextFlags_ReadOnly);
    recalculate &= ImGui::DragFloat("Angle (deg)", &sl->OuterRadiusDeg, 1.0f, 5.0f, 80.0f);

    if (recalculate) {
        Recalculate(sl);
    }
}

void Serialize(SerdeArchive* sa, SpotlightComponent* sl) {
    SERDE(sa, sl, Target);
    SERDE(sa, sl, Color);
    SERDE(sa, sl, MinCutoffDistance);
    SERDE(sa, sl, MaxCutoffDistance);
    SERDE(sa, sl, InnerRadiusDeg);
    SERDE(sa, sl, OuterRadiusDeg);
    // Note: RS_ViewPosition, RS_ViewDirection, RS_InnerRadiusCos, and RS_OuterRadiusCos are render
    // state cache, no need to serialize them
}

Vec3 GetDirection(const SpotlightComponent& sl) {
    const Entity* owner = sl.GetOwner();
    ASSERT(owner);
    return Normalize(sl.Target - owner->Transform.Position);
}

Transform& GetTransform(Light* light) {
    switch (light->LightType) {
        case ELightType::Invalid: ASSERT(false); break;
        case ELightType::Point: return light->PointLight->GetOwner()->Transform;
        case ELightType::Directional: return light->DirectionalLight->GetOwner()->Transform;
        case ELightType::Spotlight: return light->Spotlight->GetOwner()->Transform;
        case ELightType::COUNT: ASSERT(false); break;
    }

    ASSERT(false);
    static Transform kEmptyTransform = {};
    return kEmptyTransform;
}

void BuildImGui(Light* light) {
    if (light == nullptr) {
        return;
    }

    const char* current_type = ToString(light->LightType);
    if (ImGui::BeginCombo("Light Type", current_type)) {
        for (int i = 0; i < static_cast<int>(ELightType::COUNT); i++) {
            ELightType type = static_cast<ELightType>(i);
            if (type != ELightType::Invalid && type != ELightType::COUNT) {
                bool is_selected = (type == light->LightType);
                if (ImGui::Selectable(ToString(type), is_selected)) {
                    light->LightType = type;
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
        }
        ImGui::EndCombo();
    }

    switch (light->LightType) {
        case ELightType::Point: BuildImGui(light->PointLight); break;
        case ELightType::Directional: BuildImGui(light->DirectionalLight); break;
        case ELightType::Spotlight: BuildImGui(light->Spotlight); break;
        default: break;
    }
}

}  // namespace kdk
