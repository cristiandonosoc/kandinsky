// #version 330 core

#ifdef VERTEX_SHADER

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragUV;

uniform mat4 uM_ViewModel;
uniform mat4 uM_Normal;
uniform mat4 uM_Proj;

void main() {
    // In projection space.
    gl_Position = uM_Proj * uM_ViewModel * vec4(aPos, 1.0f);

    // We want the fragment position in view space.
    fragPosition = vec3(uM_ViewModel * vec4(aPos, 1.0f));

    // We want the normal in view space.
    fragNormal = mat3(uM_Normal) * aNormal;

    fragUV = aUV;
}

#endif  // VERTEX_SHADER

#ifdef FRAGMENT_SHADER

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragUV;

out vec4 FragColor;
layout(std430, binding = 0) buffer OutputBuffer {
    int ObjectID;
    float ObjectDepth;
};

uniform float uSeconds;

struct Material {
    sampler2D TextureDiffuse1;
    sampler2D TextureDiffuse2;
    sampler2D TextureDiffuse3;
    sampler2D TextureSpecular1;
    sampler2D TextureSpecular2;
    sampler2D TextureEmissive1;

    vec3 Albedo;
    vec3 Diffuse;
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

struct Spotlight {
    vec3 ViewPosition;
    vec3 ViewDirection;
    LightColor Color;

    float MinCutoffDistance;
    float MaxCutoffDistance;
    float InnerRadiusCos;
    float OuterRadiusCos;
};
uniform Spotlight uSpotlight;

uniform vec2 uMouseCoords;
uniform int uObjectID;

vec3 EvaluateLightEquation(vec3 light_dir, LightColor light_color, float attenuation) {
    vec3 diffuse_tex_value = vec3(texture(uMaterial.TextureDiffuse1, fragUV));
    // vec3 diffuse_tex_value = vec3(texture(uMaterial.TextureSpecular1, fragUV));
    vec3 specular_tex_value = vec3(texture(uMaterial.TextureSpecular1, fragUV));
    vec2 emissive_uv = fragUV + vec2(0.0, uSeconds);
    vec3 emissive_tex_value = vec3(texture(uMaterial.TextureEmissive1, emissive_uv));

    // Ambient.
    vec3 ambient = (uMaterial.Albedo + diffuse_tex_value) * light_color.Ambient;

    // Diffuse.
    vec3 normal = normalize(fragNormal);
    float diff_coef = max(dot(normal, light_dir), 0.0f);
    vec3 diffuse = (diff_coef * (uMaterial.Diffuse + diffuse_tex_value)) * light_color.Diffuse;

    // Specular.
    vec3 camera_dir = normalize(-fragPosition);
    // vec3 halfway_dir = normalize(light_dir - camera_dir);
    vec3 reflect_dir = normalize(reflect(-light_dir, normal));
    float spec_coef = pow(max(dot(camera_dir, reflect_dir), 0.0), uMaterial.Shininess);
    float spec_scale = 1.0f;
    vec3 specular = spec_scale * (spec_coef * specular_tex_value) * light_color.Specular;

    // Emissive.
    // float emissive_coef = (sin(uSeconds) + 1.0f) / 2.0f;
    float emissive_coef = 1.0f;
    // Only issue emissive where the specular texture is zero.
    vec3 emissive = emissive_tex_value * floor(vec3(1.0f) - specular_tex_value);
    emissive *= emissive_coef;

    // clang-format off
    vec3 result = vec3(0) +
				  attenuation * ambient +
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

vec3 EvaluateSpotlight(Spotlight spotlight) {
    vec3 light_position = spotlight.ViewPosition;
    float light_distance = length(light_position - fragPosition);
    vec3 light_dir = normalize(light_position - fragPosition);
    vec3 spotlight_dir = normalize(spotlight.ViewDirection);

    float theta = dot(light_dir, -spotlight_dir);

    float outer = spotlight.OuterRadiusCos;
    float inner = spotlight.InnerRadiusCos;

    vec3 value = EvaluateLightEquation(light_dir, spotlight.Color, 1.0f);
    float intensity = clamp((outer - theta) / (outer - inner), 0.0f, 1.0f);
    // TODO(cdc): Precalculate the coef. rather than making the calculation on each pixel.
    value *= clamp((spotlight.MaxCutoffDistance - light_distance) /
                       (spotlight.MaxCutoffDistance - spotlight.MinCutoffDistance),
                   0.0f,
                   1.0f);

    return value * intensity;
}

void main() {
    vec3 result = vec3(0);

    result += EvaluateDirectionalLight(uDirectionalLight);

    for (int i = 0; i < NUM_POINT_LIGHTS; i++) {
        result += EvaluatePointLight(uPointLights[i]);
    }

    // result += EvaluateSpotlight(uSpotlight);

    FragColor = vec4(result, 1.0f);

    // Evaluate the object ID SSBO.
    if (floor(gl_FragCoord.xy) == floor(uMouseCoords)) {
        FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
        if (gl_FragCoord.z < ObjectDepth) {
            ObjectDepth = gl_FragCoord.z;
            ObjectID = uObjectID;
        }
    }
}

#endif  // FRAGMENT_SHADER
