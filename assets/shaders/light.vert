#version 330 core

layout(location = 0) in vec3 aPos;

out vec3 fragPos;

uniform mat4 uModel;
uniform mat4 uViewProj;

void main() {
    gl_Position = uViewProj * uModel * vec4(aPos, 1.0);
    fragPos = vec3(gl_Position);
}
