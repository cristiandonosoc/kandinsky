#version 330 core

out vec4 FragColor;

in vec3 fragPosition;
in vec3 fragNormal;

uniform sampler2D uTex1;
uniform sampler2D uTex2;

struct Material {
    vec3 Ambient;
    vec3 Diffuse;
    vec3 Specular;
    float Shininess;
};
uniform Material uMaterial;

struct Light {
    vec3 ViewPosition;

    vec3 Ambient;
    vec3 Diffuse;
    vec3 Specular;
};
uniform Light uLight;

void main() {
    // Ambient.
    vec3 ambient = uMaterial.Ambient * uLight.Ambient;

    // Diffuse.
    vec3 normal = normalize(fragNormal);
    // NOTE: This is actually the inverse of the light direction (from frag to the light).
    vec3 light_dir = normalize(uLight.ViewPosition - fragPosition);
    float diff_coef = max(dot(normal, light_dir), 0.0f);
    vec3 diffuse = (diff_coef * uMaterial.Diffuse) * uLight.Diffuse;

    // Specular.
    vec3 camera_dir = normalize(-fragPosition);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec_coef = pow(max(dot(camera_dir, reflect_dir), 0.0), uMaterial.Shininess);
    vec3 specular = (spec_coef * uMaterial.Specular) * uLight.Specular;

    vec3 result = (ambient + diffuse + specular);
    FragColor = vec4(result, 1.0f);
}
