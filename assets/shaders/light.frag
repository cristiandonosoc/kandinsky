#version 430 core

in vec3 fragPos;

out vec4 FragColor;


layout(std430, binding = 0) buffer OutputBuffer {
    uvec2 ObjectID;
    float ObjectDepth;
};

uniform vec2 uMouseCoords;
uniform uvec2 uObjectID;
uniform vec3 uColor;

void main() {
    FragColor = vec4(uColor, 1.0f);

    // Evaluate the object ID SSBO.
    if (floor(gl_FragCoord.xy) == floor(uMouseCoords)) {
        FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
        if (gl_FragCoord.z < ObjectDepth) {
            ObjectDepth = gl_FragCoord.z;
            ObjectID = uObjectID;
        }
    }
}
