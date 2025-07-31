
//#version 330 core

#ifdef VERTEX_SHADER

layout(location = 0) in vec3 aPos;

uniform mat4 uM_ViewProj;

void main() {
	gl_Position = uM_ViewProj * vec4(aPos, 1.0);
}

#endif // VERTEX_SHADER

#ifdef FRAGMENT_SHADER

uniform vec4 uColor;

out vec4 FragColor;

void main() { FragColor = uColor; }

#endif // FRAGMENT_SHADER
