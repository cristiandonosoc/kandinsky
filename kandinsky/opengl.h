#pragma once

#include <kandinsky/defines.h>

// clang-format off
// We need this header ordering sadly.
#include <GL/glew.h>
#include <SDL3/SDL_opengl.h>
#include <GL/GLU.h>
// clang-format on

namespace kdk {

// Shader ------------------------------------------------------------------------------------------

struct Shader {
    GLuint Program = GL_NONE;
};

inline bool IsValid(const Shader& shader) { return shader.Program != GL_NONE; }

Shader CreateShader(const char* vs_path, const char* fs_path);
Shader CreateShaderFromString(const char* vs_source, const char* fs_source);

void Use(const Shader& shader);
void SetBool(const Shader& shader, const char* uniform, bool value);
void SetI32(const Shader& shader, const char* uniform, i32 value);
void SetU32(const Shader& shader, const char* uniform, u32 value);
void SetFloat(const Shader& shader, const char* uniform, float value);

// Texture -----------------------------------------------------------------------------------------

struct Texture {
    i32 Width = 0;
    i32 Height = 0;
    GLuint Handle = GL_NONE;
};
bool IsValid(const Texture& texture);


struct TextureLoadOptions {
	bool FlipVertically = false;
};
Texture LoadTexture(const char* path, const TextureLoadOptions& options = {});
void Bind(const Texture& texture, GLuint texture_unit);

}  // namespace kdk
