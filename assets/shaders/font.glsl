// #version 430 core

#ifdef VERTEX_SHADER

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aUV;

out vec4 fragColor;
out vec2 fragUV;

uniform mat4 uM_ProjViewModel;

void main() {
    gl_Position = uM_ProjViewModel * vec4(aPosition, 1.0);

    fragColor = aColor;
    fragUV = aUV;
}

#endif  // VERTEX_SHADER

#ifdef FRAGMENT_SHADER

in vec4 fragColor;
in vec2 fragUV;

out vec4 FragColor;

uniform sampler2D uFontAtlasTexture;

void main() {
    float t = texture(uFontAtlasTexture, fragUV).r;
    vec4 color = vec4(t);
    FragColor = color * fragColor;
    // FragColor = vec4(texture(uFontAtlasTexture, fragUV).r) * fragColor;
}

#endif  // FRAGMENT_SHADER
