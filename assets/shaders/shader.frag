#version 330 core

out vec4 FragColor;

in vec2 vertexUV;

uniform sampler2D uTex1;
uniform sampler2D uTex2;

uniform vec3 uObjectColor;
uniform vec3 uLightColor;

// void main() { FragColor = mix(texture(uTex1, vertexUV), texture(uTex2, vertexUV), 0.2); }

void main() { FragColor = vec4(uLightColor * uObjectColor, 1.0); }
