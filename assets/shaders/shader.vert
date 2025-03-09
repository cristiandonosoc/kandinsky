#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragUV;

uniform mat4 uM_ViewModel;
uniform mat4 uM_Normal;
uniform mat4 uM_Proj;

void main() {
    // In projection space.
    gl_Position = uM_Proj * uM_ViewModel * vec4(aPos, 1.0f);

    // We want the fragment position in view space.
    fragPosition = vec3(uM_ViewModel * vec4(aPos, 1.0f));

    // We want the normal in view space.
    fragNormal = mat3(uM_Normal) * aNormal;

	fragUV = aUV;
}
