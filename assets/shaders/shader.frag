#version 330 core

out vec4 FragColor;

in vec3 fragPosition;
in vec3 fragNormal;

uniform sampler2D uTex1;
uniform sampler2D uTex2;

uniform vec3 uObjectColor;

uniform vec3 uLightPosition;
uniform vec3 uLightColor;
uniform vec3 uCameraPosition;

// void main() { FragColor = mix(texture(uTex1, vertexUV), texture(uTex2, vertexUV), 0.2); }

void main() {
    // Ambient.
    float ambient_strength = 0.1f;
    vec3 ambient = ambient_strength * uLightColor;

    // Diffuse.
    vec3 normal = normalize(fragNormal);
    // NOTE: This is actually the inverse of the light direction (from frag to the light).
    vec3 light_dir = normalize(uLightPosition - fragPosition);
    float diff_coef = max(dot(normal, light_dir), 0.0f);
    vec3 diffuse = diff_coef * uLightColor;

	// Specular.
	float specular_strength = 0.5f;
	int shininess = 32;

	vec3 camera_dir = normalize(uCameraPosition - fragPosition);
	vec3 reflect_dir = reflect(-light_dir, normal);
	float spec_coef = pow(max(dot(camera_dir, reflect_dir), 0.0), shininess);
	vec3 specular = specular_strength * spec_coef * uLightColor;

    vec3 result = (ambient + diffuse + specular) * uObjectColor;
    FragColor = vec4(result, 1.0f);
}
