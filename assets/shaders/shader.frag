#version 330 core

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

struct Light {
	// If w == 1, it is a position.
	// If w == 0, it is a direction.
	vec4 PosDir;

    vec3 Ambient;
    vec3 Diffuse;
    vec3 Specular;
};
uniform Light uLight;

uniform float uTime;

vec3 EvaluateLight(Light light, vec3 diffuse_tex_value, vec3 specular_tex_value) {
    // Ambient.
    vec3 ambient = diffuse_tex_value * light.Ambient;

	vec3 light_dir;
	if (light.PosDir.w == 1.0f) {
		// NOTE: This is actually the inverse of the light direction (from frag to the light).
		light_dir = normalize(vec3(light.PosDir) - fragPosition);
	} else {
		light_dir = normalize(vec3(-light.PosDir));

	}

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

    vec3 result = ambient + diffuse + specular + emission;
    return result;
}

void main() {
    vec3 diffuse_tex_value = vec3(texture(uMaterial.Diffuse, fragUV));
    vec3 specular_tex_value = vec3(texture(uMaterial.Specular, fragUV));

    vec3 result = EvaluateLight(uLight, diffuse_tex_value, specular_tex_value);
    FragColor = vec4(result, 1.0f);
}
