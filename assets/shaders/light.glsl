// #version 330 core

#ifdef VERTEX_SHADER

layout(location = 0) in vec3 aPos;

out vec3 fragPos;

uniform mat4 uM_Model;
uniform mat4 uM_ViewProj;

void main() {
    gl_Position = uM_ViewProj * uM_Model * vec4(aPos, 1.0);
    fragPos = vec3(gl_Position);
}

#endif  // VERTEX_SHADER

#ifdef FRAGMENT_SHADER

in vec3 fragPos;

out vec4 FragColor;

layout(std430, binding = 0) buffer OutputBuffer {
    int ObjectID;
    float ObjectDepth;
};

uniform vec2 uMouseCoords;
uniform vec3 uColor;
uniform int uObjectID;

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

#endif  // FRAGMENT_SHADER
