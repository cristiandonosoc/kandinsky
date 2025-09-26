// #version 420 core

#ifdef VERTEX_SHADER

uniform vec3 uBillboardPos;
uniform vec2 uBillboardSize;

uniform vec3 uCameraUpWorld;
uniform vec3 uCameraRightWorld;

uniform mat4 uM_View;
uniform mat4 uM_Proj;

// clang-format off
const vec2 kPoints[4] = vec2[4](vec2(-0.5, -0.5),
								vec2(-0.5,  0.5),
								vec2( 0.5, -0.5),
								vec2( 0.5,  0.5));
// clang-format on

const int kIndices[6] = int[6](0, 1, 3, 0, 3, 2);

out vec2 fragUV;

void main() {
    int index = kIndices[gl_VertexID];
    vec2 point = kPoints[index];

    // clang-format off
	vec3 pos = uBillboardPos +
			   uCameraRightWorld * point.x * uBillboardSize.x +
			   uCameraUpWorld * point.y * uBillboardSize.y;
    // clang-format on

    // We don't need to multiply by model space since the billboard pos is already in world space.
    gl_Position = uM_Proj * uM_View * vec4(pos, 1.0);
    fragUV = point + vec2(0.5f, 0.5f);
}

#endif  // VERTEX_SHADER

#ifdef FRAGMENT_SHADER

in vec2 fragUV;

out vec4 FragColor;

uniform sampler2D uTexture;

layout(std430, binding = 0) buffer OutputBuffer {
    int ObjectID;
    float ObjectDepth;
};

uniform vec2 uMouseCoords;
uniform int uObjectID;

void main() {
    FragColor = texture(uTexture, fragUV);

    // Evaluate the object ID SSBO.
    if (floor(gl_FragCoord.xy) == floor(uMouseCoords)) {
        FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
        if (gl_FragCoord.z < ObjectDepth) {
            ObjectDepth = gl_FragCoord.z;
            ObjectID = uObjectID;
        }
    }
}

#endif  // FRAGMENT_SHADER
