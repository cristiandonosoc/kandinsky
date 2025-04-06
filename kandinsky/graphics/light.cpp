#include <kandinsky/graphics/light.h>

#include <kandinsky/graphics/opengl.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/math.h>
#include <kandinsky/serde.h>

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

void Serialize(SerdeArchive* sa, LightColor& lc) {
    SERDE(sa, lc, Ambient);
    SERDE(sa, lc, Diffuse);
    SERDE(sa, lc, Specular);
}

// PointLight --------------------------------------------------------------------------------------

void BuildImGui(PointLight* pl) {
    ImGui::InputFloat3("Position", GetPtr(pl->Entity.Transform.Position));
    BuildImGui(&pl->Color);
    ImGui::DragFloat("MinRadius", &pl->MinRadius, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat("MaxRadius", &pl->MaxRadius, 0.01f, 0.01f, 10.0f);

    ImGui::Text("Attenuation");

    ImGui::DragFloat("Constant", &pl->AttenuationConstant, 0.001f, 0.00f, 1.0f);
    ImGui::DragFloat("Linear", &pl->AttenuationLinear, 0.001f, 0.00f, 1.0f);
    ImGui::DragFloat("Quadratic", &pl->AttenuationQuadratic, 0.001f, 0.00f, 1.0f);
}

void Serialize(SerdeArchive* sa, PointLight& pl) {
    SERDE(sa, pl, Entity);
    SERDE(sa, pl, Color);
    SERDE(sa, pl, MinRadius);
    SERDE(sa, pl, MaxRadius);
    SERDE(sa, pl, AttenuationConstant);
    SERDE(sa, pl, AttenuationLinear);
    SERDE(sa, pl, AttenuationQuadratic);
    // Note: RS_ViewPosition is a render state cache, no need to serialize it
}

void Draw(const PointLight& pl, const Shader& shader, const Mesh& mesh, const RenderState& rs) {
    Use(shader);

    Mat4 model(1.0f);
    model = Translate(model, Vec3(pl.Entity.Transform.Position));
    model = Scale(model, Vec3(0.2f));

    SetMat4(shader, "uModel", GetPtr(model));
    SetMat4(shader, "uViewProj", GetPtr(rs.M_ViewProj));

    Draw(mesh, shader, rs);
}

void BuildImGui(DirectionalLight* dl) {
    ImGui::InputFloat3("Direction", GetPtr(dl->Direction));
    BuildImGui(&dl->Color);
}

void Serialize(SerdeArchive* sa, DirectionalLight& dl) {
    SERDE(sa, dl, Entity);
    SERDE(sa, dl, Direction);
    SERDE(sa, dl, Color);
    // Note: RS_ViewDirection is a render state cache, no need to serialize it
}

// Spotlight ---------------------------------------------------------------------------------------

void Recalculate(Spotlight* sl) {
    sl->MaxCutoffDistance = Distance(sl->Entity.Transform.Position, sl->Target);
    sl->MinCutoffDistance = sl->MaxCutoffDistance * 0.9f;
    sl->InnerRadiusDeg = sl->OuterRadiusDeg * 0.9f;
}

void BuildImgui(Spotlight* sl) {
    bool recalculate = false;
    recalculate &= ImGui::InputFloat3("Position", GetPtr(sl->Entity.Transform.Position));
    recalculate &= ImGui::InputFloat3("Target", GetPtr(sl->Target));

    ImGui::InputFloat("Length", &sl->MaxCutoffDistance, ImGuiInputTextFlags_ReadOnly);
    recalculate &= ImGui::DragFloat("Angle (deg)", &sl->OuterRadiusDeg, 1.0f, 5.0f, 80.0f);

    if (recalculate) {
        Recalculate(sl);
    }
}

void Serialize(SerdeArchive* sa, Spotlight& sl) {
    SERDE(sa, sl, Entity);
    SERDE(sa, sl, Target);
    SERDE(sa, sl, Color);
    SERDE(sa, sl, MinCutoffDistance);
    SERDE(sa, sl, MaxCutoffDistance);
    SERDE(sa, sl, InnerRadiusDeg);
    SERDE(sa, sl, OuterRadiusDeg);
    // Note: RS_ViewPosition, RS_ViewDirection, RS_InnerRadiusCos, and RS_OuterRadiusCos are render
    // state cache, no need to serialize them
}

Vec3 GetDirection(const Spotlight& sl) {
    return Normalize(sl.Target - sl.Entity.Transform.Position);
}

Transform& GetTransform(Light* light) {
    switch (light->LightType) {
        case ELightType::Invalid: ASSERT(false); break;
        case ELightType::Point: return light->PointLight.Entity.Transform;
        case ELightType::Directional: return light->DirectionalLight.Entity.Transform;
        case ELightType::Spotlight: return light->Spotlight.Entity.Transform;
        case ELightType::COUNT: ASSERT(false); break;
    }

    ASSERT(false);
    static Transform kEmptyTransform = {};
    return kEmptyTransform;
}

}  // namespace kdk
