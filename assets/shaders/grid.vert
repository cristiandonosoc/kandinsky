#version 430 core

out vec3 fragPosition;

uniform float uGridSize;
uniform vec3 uCameraPos;

uniform mat4 uM_View;
uniform mat4 uM_Proj;

// clang-format off
const vec3 kPoints[4] = vec3[4](vec3(-1.0, 0.0, -1.0),
                                vec3(-1.0, 0.0,  1.0),
                                vec3( 1.0, 0.0, -1.0),
                                vec3( 1.0, 0.0,  1.0));
// clang-format on

const int kIndices[6] = int[6](0, 1, 3, 0, 3, 2);


void main() {
	int index = kIndices[gl_VertexID];
	vec3 pos = kPoints[index];


	pos *= uGridSize;
	pos.x += uCameraPos.x;
	pos.z += uCameraPos.z;

	fragPosition = pos;
	gl_Position = uM_Proj * uM_View * vec4(pos, 1.0f);
}
