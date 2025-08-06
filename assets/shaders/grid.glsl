// #version 430 core

#ifdef VERTEX_SHADER

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

#endif

#ifdef FRAGMENT_SHADER

in vec3 fragPosition;

out vec4 FragColor;

uniform vec3 uCameraPos;
uniform vec2 uFogRange;

void main() {
    vec2 wrapped = abs(fract(fragPosition.xz) - vec2(0.5f, 0.5f));
    vec2 speed = fwidth(fragPosition.xz);

    vec2 range = wrapped / speed;

    float line_width = 0.05f;
    float weight = clamp(min(range.x, range.y) - line_width, 0.0f, 1.0f);

    float camera_dist = distance(uCameraPos, fragPosition);

    float fog = 1 - ((camera_dist - uFogRange.x) / (uFogRange.y - uFogRange.x));

    float grid_weight = 1 - weight;

    float result = grid_weight * fog;
    if (abs(result) > 0.0001f) {
        FragColor = vec4(0, 0, 0, grid_weight * fog);
    } else {
        discard;
    }
}

#endif
