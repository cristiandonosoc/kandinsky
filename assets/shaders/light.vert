#version 330 core

layout(location = 0) in vec3 aPos;

out vec3 fragPos;

uniform mat4 uM_Model;
uniform mat4 uM_ViewProj;

void main() {
    gl_Position = uM_ViewProj * uM_Model * vec4(aPos, 1.0);
    fragPos = vec3(gl_Position);
}
