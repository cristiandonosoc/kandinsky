#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 uM_ViewProj;

void main() {
	gl_Position = uM_ViewProj * vec4(aPos, 1.0);
}
