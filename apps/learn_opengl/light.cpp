#include <learn_opengl/light.h>

#include <kandinsky/math.h>
#include <kandinsky/opengl.h>

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
    model = glm::translate(model, Vec3(pl.Position));
    model = glm::scale(model, Vec3(0.2f));

    SetMat4(shader, "uModel", GetPtr(model));
    SetMat4(shader, "uViewProj", GetPtr(*rs.MatViewProj));

    DrawMesh(mesh, shader, rs);
}

void BuildImGui(DirectionalLight* dl) {
    ImGui::InputFloat3("Direction", GetPtr(dl->Direction));
    BuildImGui(&dl->Color);
}

void DrawMesh(const Mesh& mesh, const Shader& shader, const RenderState& rs) {
    Use(shader);
    SetFloat(shader, "uSeconds", rs.Seconds);

    SetMat4(shader, "uProj", GetPtr(*rs.MatProj));

    SetVec3(shader, "uDirectionalLight.ViewDirection", rs.DirectionalLight.ViewDirection);
    SetVec3(shader, "uDirectionalLight.Color.Ambient", rs.DirectionalLight.DL->Color.Ambient);
    SetVec3(shader, "uDirectionalLight.Color.Diffuse", rs.DirectionalLight.DL->Color.Diffuse);
    SetVec3(shader, "uDirectionalLight.Color.Specular", rs.DirectionalLight.DL->Color.Specular);

    // clang-format off
	SetVec3(shader,  "uPointLights[0].ViewPosition", rs.PointLights[0].ViewPosition);
    SetVec3(shader,  "uPointLights[0].Color.Ambient", rs.PointLights[0].PL->Color.Ambient);
    SetVec3(shader,  "uPointLights[0].Color.Diffuse", rs.PointLights[0].PL->Color.Diffuse);
    SetVec3(shader,  "uPointLights[0].Color.Specular", rs.PointLights[0].PL->Color.Specular);
    SetFloat(shader, "uPointLights[0].MinRadius", rs.PointLights[0].PL->MinRadius);
    SetFloat(shader, "uPointLights[0].MaxRadius", rs.PointLights[0].PL-> MaxRadius);
    SetFloat(shader, "uPointLights[0].AttenuationConstant", rs.PointLights[0].PL-> AttenuationConstant);
    SetFloat(shader, "uPointLights[0].AttenuationLinear", rs.PointLights[0].PL-> AttenuationLinear);
    SetFloat(shader, "uPointLights[0].AttenuationQuadratic", rs.PointLights[0].PL-> AttenuationQuadratic);

	SetVec3(shader,  "uPointLights[1].ViewPosition", rs.PointLights[1].ViewPosition);
    SetVec3(shader,  "uPointLights[1].Color.Ambient", rs.PointLights[1].PL->Color.Ambient);
    SetVec3(shader,  "uPointLights[1].Color.Diffuse", rs.PointLights[1].PL->Color.Diffuse);
    SetVec3(shader,  "uPointLights[1].Color.Specular", rs.PointLights[1].PL->Color.Specular);
    SetFloat(shader, "uPointLights[1].MinRadius", rs.PointLights[1].PL->MinRadius);
    SetFloat(shader, "uPointLights[1].MaxRadius", rs.PointLights[1].PL-> MaxRadius);
    SetFloat(shader, "uPointLights[1].AttenuationConstant", rs.PointLights[1].PL-> AttenuationConstant);
    SetFloat(shader, "uPointLights[1].AttenuationLinear", rs.PointLights[1].PL-> AttenuationLinear);
    SetFloat(shader, "uPointLights[1].AttenuationQuadratic", rs.PointLights[1].PL-> AttenuationQuadratic);

	SetVec3(shader,  "uPointLights[2].ViewPosition", rs.PointLights[2].ViewPosition);
    SetVec3(shader,  "uPointLights[2].Color.Ambient", rs.PointLights[2].PL->Color.Ambient);
    SetVec3(shader,  "uPointLights[2].Color.Diffuse", rs.PointLights[2].PL->Color.Diffuse);
    SetVec3(shader,  "uPointLights[2].Color.Specular", rs.PointLights[2].PL->Color.Specular);
    SetFloat(shader, "uPointLights[2].MinRadius", rs.PointLights[2].PL->MinRadius);
    SetFloat(shader, "uPointLights[2].MaxRadius", rs.PointLights[2].PL-> MaxRadius);
    SetFloat(shader, "uPointLights[2].AttenuationConstant", rs.PointLights[2].PL-> AttenuationConstant);
    SetFloat(shader, "uPointLights[2].AttenuationLinear", rs.PointLights[2].PL-> AttenuationLinear);
    SetFloat(shader, "uPointLights[2].AttenuationQuadratic", rs.PointLights[2].PL-> AttenuationQuadratic);

	SetVec3(shader,  "uPointLights[3].ViewPosition", rs.PointLights[3].ViewPosition);
    SetVec3(shader,  "uPointLights[3].Color.Ambient", rs.PointLights[3].PL->Color.Ambient);
    SetVec3(shader,  "uPointLights[3].Color.Diffuse", rs.PointLights[3].PL->Color.Diffuse);
    SetVec3(shader,  "uPointLights[3].Color.Specular", rs.PointLights[3].PL->Color.Specular);
    SetFloat(shader, "uPointLights[3].MinRadius", rs.PointLights[3].PL->MinRadius);
    SetFloat(shader, "uPointLights[3].MaxRadius", rs.PointLights[3].PL-> MaxRadius);
    SetFloat(shader, "uPointLights[3].AttenuationConstant", rs.PointLights[3].PL-> AttenuationConstant);
    SetFloat(shader, "uPointLights[3].AttenuationLinear", rs.PointLights[3].PL-> AttenuationLinear);
    SetFloat(shader, "uPointLights[3].AttenuationQuadratic", rs.PointLights[3].PL-> AttenuationQuadratic);
    // clang-format on

    Draw(mesh, shader);
}

}  // namespace kdk
