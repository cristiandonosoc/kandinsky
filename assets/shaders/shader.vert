#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
// layout(location = 2) in vec2 aUV;

// out vec2 vertexUV;

out vec3 fragPosition;
out vec3 fragNormal;
out vec3 fragLightPosition;

// This should be likely calculated on the CPU.
uniform vec3 uLightPosition;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main() {
    mat4 view_model = uView * uModel;

    // In projection space.
    gl_Position = uProj * view_model * vec4(aPos, 1.0f);

    // We want the fragment position in view space.
    fragPosition = vec3(view_model * vec4(aPos, 1.0f));

    // We want the normal in view space.
    // NOTE: This should be done on the CPU.
    fragNormal = mat3(transpose(inverse(view_model))) * aNormal;

    // We want the light in view space.
    fragLightPosition = vec3(uView * vec4(uLightPosition, 1.0f));
}
