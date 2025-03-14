#include <kandinsky/graphics/render_state.h>

#include <kandinsky/camera.h>
#include <kandinsky/graphics/opengl.h>

namespace kdk {

void SetCamera(RenderState* rs, const Camera& camera) {
    rs->CameraPosition = camera.Position;
    rs->M_View = camera.M_View;
    rs->M_Proj = camera.M_Proj;
    rs->M_ViewProj = camera.M_ViewProj;
}

void SetLights(RenderState* rs, std::span<Light*> lights) {
    for (Light* light : lights) {
        const Transform& transform = light->GetTransform();
        switch (light->LightType) {
            case ELightType::Invalid: ASSERT(false); return;
            case ELightType::Point: {
                light->PointLight.RS_ViewPosition = rs->M_View * Vec4(transform.Position, 1.0f);
                break;
            }
            case ELightType::Directional: {
                light->DirectionalLight.RS_ViewDirection =
                    rs->M_View * Vec4(light->DirectionalLight.Direction, 0.0f);
                break;
            }
            case ELightType::Spotlight: {
                light->Spotlight.RS_ViewPosition = rs->M_View * Vec4(transform.Position, 1.0f);
                Vec3 spotlight_dir = light->Spotlight.Target - transform.Position;
                light->Spotlight.RS_ViewDirection = rs->M_View * Vec4(spotlight_dir, 0.0f);
                light->Spotlight.RS_InnerRadiusCos =
                    Cos(ToRadians(light->Spotlight.InnerRadiusDeg));
                light->Spotlight.RS_OuterRadiusCos =
                    Cos(ToRadians(light->Spotlight.OuterRadiusDeg));
                break;
            }
            case ELightType::COUNT: ASSERT(false); return;
        }
    }

    rs->Lights = lights;
}

void ChangeModelMatrix(RenderState* rs, const Mat4& mmodel) {
    rs->M_Model = mmodel;
    rs->M_ViewModel = rs->M_View * rs->M_Model;
    rs->M_Normal = Transpose(Inverse(rs->M_ViewModel));
}

void SetUniforms(const RenderState& rs, const Shader& shader) {
    Use(shader);
    SetFloat(shader, "uSeconds", rs.Seconds);

    SetMat4(shader, "uM_Model", GetPtr(rs.M_Model));
    SetMat4(shader, "uM_Normal", GetPtr(rs.M_Normal));
    SetMat4(shader, "uM_View", GetPtr(rs.M_View));
    SetMat4(shader, "uM_ViewModel", GetPtr(rs.M_ViewModel));
    SetMat4(shader, "uM_ViewProj", GetPtr(rs.M_ViewProj));
    SetMat4(shader, "uM_Proj", GetPtr(rs.M_Proj));

    u32 dir_light_count = 0;
    u32 point_light_count = 0;
    u32 spot_light_count = 0;

    // clang-format off
	for (Light* light : rs.Lights) {
		if (light->LightType == ELightType::Directional) {
			if (dir_light_count > 1) {
				continue;
			}


			SetVec3(shader, "uDirectionalLight.ViewDirection", light->DirectionalLight.RS_ViewDirection);
			SetVec3(shader, "uDirectionalLight.Color.Ambient", light->DirectionalLight.Color.Ambient);
			SetVec3(shader, "uDirectionalLight.Color.Diffuse", light->DirectionalLight.Color.Diffuse);
			SetVec3(shader, "uDirectionalLight.Color.Specular", light->DirectionalLight.Color.Specular);
			dir_light_count++;
			continue;
		}

		if (light->LightType == ELightType::Point) {
			if (point_light_count > 3) {
				continue;
			}

			if (point_light_count == 0) {
				SetVec3(shader,  "uPointLights[0].ViewPosition", light->PointLight.RS_ViewPosition);
				SetVec3(shader,  "uPointLights[0].Color.Ambient", light->PointLight.Color.Ambient);
				SetVec3(shader,  "uPointLights[0].Color.Diffuse", light->PointLight.Color.Diffuse);
				SetVec3(shader,  "uPointLights[0].Color.Specular", light->PointLight.Color.Specular);
				SetFloat(shader, "uPointLights[0].MinRadius", light->PointLight.MinRadius);
				SetFloat(shader, "uPointLights[0].MaxRadius", light->PointLight.MaxRadius);
				SetFloat(shader, "uPointLights[0].AttenuationConstant", light->PointLight.AttenuationConstant);
				SetFloat(shader, "uPointLights[0].AttenuationLinear", light->PointLight.AttenuationLinear);
				SetFloat(shader, "uPointLights[0].AttenuationQuadratic", light->PointLight.AttenuationQuadratic);
			} else if (point_light_count == 1) {
				SetVec3(shader,  "uPointLights[1].ViewPosition", light->PointLight.RS_ViewPosition);
				SetVec3(shader,  "uPointLights[1].Color.Ambient", light->PointLight.Color.Ambient);
				SetVec3(shader,  "uPointLights[1].Color.Diffuse", light->PointLight.Color.Diffuse);
				SetVec3(shader,  "uPointLights[1].Color.Specular", light->PointLight.Color.Specular);
				SetFloat(shader, "uPointLights[1].MinRadius", light->PointLight.MinRadius);
				SetFloat(shader, "uPointLights[1].MaxRadius", light->PointLight.MaxRadius);
				SetFloat(shader, "uPointLights[1].AttenuationConstant", light->PointLight.AttenuationConstant);
				SetFloat(shader, "uPointLights[1].AttenuationLinear", light->PointLight.AttenuationLinear);
				SetFloat(shader, "uPointLights[1].AttenuationQuadratic", light->PointLight.AttenuationQuadratic);
			} else if (point_light_count == 2) {
				SetVec3(shader,  "uPointLights[2].ViewPosition", light->PointLight.RS_ViewPosition);
				SetVec3(shader,  "uPointLights[2].Color.Ambient", light->PointLight.Color.Ambient);
				SetVec3(shader,  "uPointLights[2].Color.Diffuse", light->PointLight.Color.Diffuse);
				SetVec3(shader,  "uPointLights[2].Color.Specular", light->PointLight.Color.Specular);
				SetFloat(shader, "uPointLights[2].MinRadius", light->PointLight.MinRadius);
				SetFloat(shader, "uPointLights[2].MaxRadius", light->PointLight.MaxRadius);
				SetFloat(shader, "uPointLights[2].AttenuationConstant", light->PointLight.AttenuationConstant);
				SetFloat(shader, "uPointLights[2].AttenuationLinear", light->PointLight.AttenuationLinear);
				SetFloat(shader, "uPointLights[2].AttenuationQuadratic", light->PointLight.AttenuationQuadratic);
			} else if (point_light_count == 3) {
				SetVec3(shader,  "uPointLights[3].ViewPosition", light->PointLight.RS_ViewPosition);
				SetVec3(shader,  "uPointLights[3].Color.Ambient", light->PointLight.Color.Ambient);
				SetVec3(shader,  "uPointLights[3].Color.Diffuse", light->PointLight.Color.Diffuse);
				SetVec3(shader,  "uPointLights[3].Color.Specular", light->PointLight.Color.Specular);
				SetFloat(shader, "uPointLights[3].MinRadius", light->PointLight.MinRadius);
				SetFloat(shader, "uPointLights[3].MaxRadius", light->PointLight.MaxRadius);
				SetFloat(shader, "uPointLights[3].AttenuationConstant", light->PointLight.AttenuationConstant);
				SetFloat(shader, "uPointLights[3].AttenuationLinear", light->PointLight.AttenuationLinear);
				SetFloat(shader, "uPointLights[3].AttenuationQuadratic", light->PointLight.AttenuationQuadratic);
			}

			point_light_count++;;
			continue;
		}

		if (light->LightType == ELightType::Spotlight) {
			if (spot_light_count == 0) {
				continue;
			}

			SetVec3(shader, "uSpotlight.ViewPosition", light->Spotlight.RS_ViewPosition);
			SetVec3(shader, "uSpotlight.ViewDirection", light->Spotlight.RS_ViewDirection);
			SetFloat(shader, "uSpotlight.InnerRadiusCos", light->Spotlight.RS_InnerRadiusCos);
			SetFloat(shader, "uSpotlight.OuterRadiusCos", light->Spotlight.RS_OuterRadiusCos);
			SetVec3(shader, "uSpotlight.Color.Ambient", light->Spotlight.Color.Ambient);
			SetVec3(shader, "uSpotlight.Color.Diffuse", light->Spotlight.Color.Diffuse);
			SetVec3(shader, "uSpotlight.Color.Specular", light->Spotlight.Color.Specular);
			SetFloat(shader, "uSpotlight.MinCutoffDistance", light->Spotlight.MinCutoffDistance);
			SetFloat(shader, "uSpotlight.MaxCutoffDistance", light->Spotlight.MaxCutoffDistance);

			spot_light_count++;
		}
	}
    // clang-format on
}

}  // namespace kdk
