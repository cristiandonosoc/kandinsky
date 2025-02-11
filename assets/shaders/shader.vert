#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
// layout(location = 2) in vec2 aUV;

// out vec2 vertexUV;

out vec3 fragPosition;
out vec3 fragNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main() {
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0f);
    // We want the fragment position in world space.
    fragPosition = vec3(uModel * vec4(aPos, 1.0f));
    // NOTE: This should be done on the CPO.
    fragNormal = mat3(transpose(inverse(uModel))) * aNormal;
}
