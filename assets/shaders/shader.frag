#version 420 core

out vec4 FragColor;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragUV;

uniform float uSeconds;

struct Material {
    sampler2D TextureDiffuse1;
    sampler2D TextureDiffuse2;
    sampler2D TextureDiffuse3;
    sampler2D TextureSpecular1;
    sampler2D TextureSpecular2;
    sampler2D TextureEmissive1;

    float Shininess;
};
uniform Material uMaterial;

struct LightColor {
    vec3 Ambient;
    vec3 Diffuse;
    vec3 Specular;
};

struct DirectionalLight {
    vec3 ViewDirection;
    LightColor Color;
};
uniform DirectionalLight uDirectionalLight;

struct PointLight {
    vec3 ViewPosition;
    LightColor Color;

    float MinRadius;
    float MaxRadius;
    float AttenuationConstant;
    float AttenuationLinear;
    float AttenuationQuadratic;
};
#define NUM_POINT_LIGHTS 4
uniform PointLight uPointLights[NUM_POINT_LIGHTS];

// struct Spotlight {
//     vec3 ViewPosition;
//     vec3 ViewDirection;
//     float MinCutoffDistance;
//     float MaxCutoffDistance;
//     float InnerRadiusCos;
//     float OuterRadiusCos;
// };
// uniform Spotlight uSpotlight;

vec3 EvaluateLightEquation(vec3 light_dir, LightColor light_color, float attenuation) {
    vec3 diffuse_tex_value = vec3(texture(uMaterial.TextureDiffuse1, fragUV));
    vec3 specular_tex_value = vec3(texture(uMaterial.TextureSpecular1, fragUV));
    vec2 emissive_uv = fragUV + vec2(0.0, uSeconds);
    vec3 emissive_tex_value = vec3(texture(uMaterial.TextureEmissive1, emissive_uv));

    // Ambient.
    vec3 ambient = diffuse_tex_value * light_color.Ambient;

    // Diffuse.
    vec3 normal = normalize(fragNormal);
    float diff_coef = max(dot(normal, light_dir), 0.0f);
    vec3 diffuse = (diff_coef * diffuse_tex_value) * light_color.Diffuse;

    // Specular.
    vec3 camera_dir = normalize(-fragPosition);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec_coef = pow(max(dot(camera_dir, reflect_dir), 0.0), uMaterial.Shininess);
    vec3 specular = (spec_coef * specular_tex_value) * light_color.Specular;

    // Emissive.
    // float emissive_coef = (sin(uSeconds) + 1.0f) / 2.0f;
    float emissive_coef = 1.0f;
    // Only issue emissive where the specular texture is zero.
    vec3 emissive = emissive_tex_value * floor(vec3(1.0f) - specular_tex_value);
    emissive *= emissive_coef;

    // clang-format off
    vec3 result = attenuation * ambient +
				  attenuation * diffuse +
				  attenuation * specular +
				  //emissive +
				  vec3(0);
    // clang-format on

    return result;
}

vec3 EvaluateDirectionalLight(DirectionalLight dl) {
    float attenuation = 1.0f;
    return EvaluateLightEquation(-dl.ViewDirection, dl.Color, attenuation);
}

// https://gamedev.stackexchange.com/questions/51291/deferred-rendering-and-point-light-radius
//
// Something to evaluate for later:
// https://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/
vec3 EvaluatePointLight(PointLight pl) {
    // NOTE: This is actually the inverse of the light direction (from frag to the light).
    vec3 light_dir = normalize(pl.ViewPosition - fragPosition);
    float light_distance = length(pl.ViewPosition - fragPosition);

    // clang-format off
	float attenuation = 1.0f / (pl.AttenuationConstant +
								pl.AttenuationLinear * light_distance +
								pl.AttenuationQuadratic * (light_distance, light_distance));
    // clang-format on

    vec3 value = EvaluateLightEquation(light_dir, pl.Color, attenuation);

    // TODO(cdc): Precalculate the coef. rather than making the calculation on each pixel.
    value *= clamp((pl.MaxRadius - light_distance) / (pl.MaxRadius - pl.MinRadius), 0.0f, 1.0f);
	value = max(value, vec3(0));
    return value;
}

// vec3 EvaluateSpotlight(Light light) {
//     vec3 light_position = vec3(light.PosDir);
//     vec3 light_dir = normalize(light_position - fragPosition);
//     vec3 spotlight_dir = normalize(light.Spotlight.Direction);

//     float theta = dot(light_dir, -spotlight_dir);

//     float outer = light.Spotlight.OuterRadiusCos;
//     float inner = light.Spotlight.InnerRadiusCos;

//     vec3 value = EvaluateLightEquation(light_dir, light, 1.0f);
//     float intensity = clamp((outer - theta) / (outer - inner), 0.0f, 1.0f);

//     return value * intensity;

//     // if (theta > light.Spotlight.Cutoff) {
//     //     return vec3(1.0f, 0.0f, 1.0f);
//     // } else {
//     //     return vec3(0.0f, 0.0f, 0.0f);
//     // }
// }

void main() {
    vec3 result = vec3(0);

    result += EvaluateDirectionalLight(uDirectionalLight);

	for (int i = 0; i < NUM_POINT_LIGHTS; i++) {
		result += EvaluatePointLight(uPointLights[i]);
	}

    // if (uLight.PosDir.w == 0.0f) {
    //     result = EvaluateDirectionalLight(uLight);
    // } else if (uLight.PosDir.w == 1.0f) {
    //     result = EvaluatePointLight(uLight);
    // } else if (uLight.PosDir.w == 2.0f) {
    //     result = EvaluateSpotlight(uLight);
    // } else {
    //     // Error color.
    //     result = vec3(0.8f, 0.0f, 0.8f);
    // }

    FragColor = vec4(result, 1.0f);
}
