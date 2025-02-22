#version 420 core

out vec4 FragColor;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragUV;

struct Material {
    sampler2D Diffuse;
    sampler2D Specular;
    sampler2D Emission;
    float Shininess;
};
uniform Material uMaterial;

struct AttenuationParams {
    float MinRadius;
    float MaxRadius;
    float Constant;
    float Linear;
    float Quadratic;
};

struct AttenuationResult {
    float Ambient;
    float Diffuse;
    float Specular;
};

struct SpotlightParams {
    vec3 Direction;
    float Cutoff;
};

struct Light {
    // If w == 0, it is a direction.
    // If w == 1, it is a position.
    // If w == 2, it is a spotlight.
    vec4 PosDir;

    vec3 Ambient;
    vec3 Diffuse;
    vec3 Specular;

    AttenuationParams Attenuation;
    SpotlightParams Spotlight;
};
uniform Light uLight;

uniform float uTime;

vec3 EvaluateLightEquation(vec3 light_dir, Light light, float attenuation) {
    vec3 diffuse_tex_value = vec3(texture(uMaterial.Diffuse, fragUV));
    vec3 specular_tex_value = vec3(texture(uMaterial.Specular, fragUV));

    // Ambient.
    vec3 ambient = diffuse_tex_value * light.Ambient;

    // Diffuse.
    vec3 normal = normalize(fragNormal);
    float diff_coef = max(dot(normal, light_dir), 0.0f);
    vec3 diffuse = (diff_coef * diffuse_tex_value) * light.Diffuse;

    // Specular.
    vec3 camera_dir = normalize(-fragPosition);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec_coef = pow(max(dot(camera_dir, reflect_dir), 0.0), uMaterial.Shininess);
    vec3 specular = (spec_coef * specular_tex_value) * light.Specular;

    // Emission.
    // Only issue emission where the specular texture is zero.
    vec2 emission_uv = fragUV + vec2(0.0, uTime);
    vec3 emission_tex_value = vec3(texture(uMaterial.Emission, emission_uv));
    float emission_coef = (sin(uTime) + 1.0f) / 2.0f;
    vec3 emission = emission_tex_value * floor(vec3(1.0f) - specular_tex_value);
    emission *= emission_coef;

    // clang-format off
    vec3 result = attenuation * ambient +
				  attenuation * diffuse +
				  attenuation * specular +
				  emission;
    // clang-format on

    return result;
}

vec3 EvaluateDirectionalLight(Light light) {
    vec3 light_dir = normalize(vec3(-light.PosDir));
    float attenuation = 1.0f;
    return EvaluateLightEquation(light_dir, light, attenuation);
}

// https://gamedev.stackexchange.com/questions/51291/deferred-rendering-and-point-light-radius
//
// Something to evaluate for later:
// https://imdoingitwrong.wordpress.com/2011/01/31/light-attenuation/
vec3 EvaluatePointLight(Light light) {
    vec3 light_position = vec3(light.PosDir);
    // NOTE: This is actually the inverse of the light direction (from frag to the light).
    vec3 light_dir = normalize(light_position - fragPosition);
    float light_distance = length(light_position - fragPosition);
    // clang-format off
	float attenuation = 1.0f / (light.Attenuation.Constant +
								light.Attenuation.Linear * light_distance +
								light.Attenuation.Quadratic * (light_distance, light_distance));
    // clang-format on

    vec3 value = EvaluateLightEquation(light_dir, light, attenuation);

    // TODO(cdc): Precalculate the coef. rather than making the calculation on each pixel.

    value *= clamp((light.Attenuation.MaxRadius - light_distance) /
                       (light.Attenuation.MaxRadius - light.Attenuation.MinRadius),
                   0.0f,
                   1.0f);

    return value;
}

vec3 EvaluateSpotlight(Light light) {
    vec3 light_position = vec3(light.PosDir);
    vec3 light_dir = normalize(light_position - fragPosition);
    vec3 spotlight_dir = normalize(light.Spotlight.Direction);

    float theta = dot(light_dir, spotlight_dir);
    if (theta > light.Spotlight.Cutoff) {
        return vec3(1.0f, 0.0f, 1.0f);
    } else {
        return vec3(0.0f, 0.0f, 0.0f);
    }
}

void main() {
    vec3 result;
    if (uLight.PosDir.w == 0.0f) {
        result = EvaluateDirectionalLight(uLight);
    } else if (uLight.PosDir.w == 1.0f) {
        result = EvaluatePointLight(uLight);
    } else if (uLight.PosDir.w == 2.0f) {
        result = EvaluateSpotlight(uLight);
    } else {
        // Error color.
        result = vec3(0.8f, 0.0f, 0.8f);
    }

    FragColor = vec4(result, 1.0f);
}
