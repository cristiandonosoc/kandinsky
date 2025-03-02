#include <kandinsky/graphics/render_state.h>

#include <kandinsky/graphics/opengl.h>

namespace kdk {

void SetUniforms(const RenderState& rs, const Shader& shader) {
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

    SetVec3(shader, "uSpotlight.ViewPosition", rs.Spotlight.ViewPosition);
    SetVec3(shader, "uSpotlight.ViewDirection", rs.Spotlight.ViewDirection);
    SetVec3(shader, "uSpotlight.Color.Ambient", rs.Spotlight.SL->Color.Ambient);
    SetVec3(shader, "uSpotlight.Color.Diffuse", rs.Spotlight.SL->Color.Diffuse);
    SetVec3(shader, "uSpotlight.Color.Specular", rs.Spotlight.SL->Color.Specular);
    SetFloat(shader, "uSpotlight.MinCutoffDistance", rs.Spotlight.SL->MinCutoffDistance);
    SetFloat(shader, "uSpotlight.MaxCutoffDistance", rs.Spotlight.SL->MaxCutoffDistance);
    SetFloat(shader, "uSpotlight.InnerRadiusCos", rs.Spotlight.InnerRadiusCos);
    SetFloat(shader, "uSpotlight.OuterRadiusCos", rs.Spotlight.OuterRadiusCos);
}

}  // namespace kdk
