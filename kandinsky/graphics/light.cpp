#include <kandinsky/graphics/light.h>

#include <kandinsky/graphics/opengl.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/math.h>

#include <imgui.h>

namespace kdk {

const char* ToString(ELightType v) {
    switch (v) {
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

// PointLight --------------------------------------------------------------------------------------

void BuildImGui(PointLight* pl) {
    ImGui::InputFloat3("Position", GetPtr(pl->Position));
    BuildImGui(&pl->Color);
    ImGui::DragFloat("MinRadius", &pl->MinRadius, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat("MaxRadius", &pl->MaxRadius, 0.01f, 0.01f, 10.0f);

    ImGui::Text("Attenuation");

    ImGui::DragFloat("Constant", &pl->AttenuationConstant, 0.001f, 0.00f, 1.0f);
    ImGui::DragFloat("Linear", &pl->AttenuationLinear, 0.001f, 0.00f, 1.0f);
    ImGui::DragFloat("Quadratic", &pl->AttenuationQuadratic, 0.001f, 0.00f, 1.0f);
}

void Draw(const PointLight& pl, const Shader& shader, const Mesh& mesh, const RenderState& rs) {
    Use(shader);

    Mat4 model(1.0f);
    model = Translate(model, Vec3(pl.Position));
    model = Scale(model, Vec3(0.2f));

    SetMat4(shader, "uModel", GetPtr(model));
    SetMat4(shader, "uViewProj", GetPtr(*rs.MatViewProj));

    Draw(mesh, shader, rs);
}

void BuildImGui(DirectionalLight* dl) {
    ImGui::InputFloat3("Direction", GetPtr(dl->Direction));
    BuildImGui(&dl->Color);
}

// Spotlight ---------------------------------------------------------------------------------------

void Recalculate(Spotlight* sl) {
    sl->MaxCutoffDistance = Distance(sl->Position, sl->Target);
    sl->MinCutoffDistance = sl->MaxCutoffDistance * 0.9f;
    sl->InnerRadiusDeg = sl->OuterRadiusDeg * 0.9f;
}

void BuildImgui(Spotlight* sl) {
    bool recalculate = false;
    recalculate &= ImGui::InputFloat3("Position", GetPtr(sl->Position));
    recalculate &= ImGui::InputFloat3("Target", GetPtr(sl->Target));

    ImGui::InputFloat("Length", &sl->MaxCutoffDistance, ImGuiInputTextFlags_ReadOnly);
    recalculate &= ImGui::DragFloat("Angle (deg)", &sl->OuterRadiusDeg, 1.0f, 5.0f, 80.0f);

    if (recalculate) {
        Recalculate(sl);
    }
}

}  // namespace kdk
