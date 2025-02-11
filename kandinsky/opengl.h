#pragma once

#include <kandinsky/defines.h>

#include <glm/glm.hpp>

// clang-format off
// We need this header ordering sadly.
#include <GL/glew.h>
#include <SDL3/SDL_opengl.h>
#include <GL/GLU.h>
// clang-format on

#include <array>
#include <span>

namespace kdk {

// Camera ------------------------------------------------------------------------------------------

struct Camera {
    glm::vec3 Position = {};
    glm::vec3 Front = {};
    glm::vec3 Up = {};
    glm::vec3 Right = {};

    // Euler angles (in radians).
    float Yaw = 0;
    float Pitch = 0;

    float MovementSpeed = 2.5f;
    float MouseSensitivity = 0.1f;
};

// In radians.
void OffsetEulerAngles(Camera* camera, float yaw, float pitch);
void Update(Camera* camera, float dt);
glm::mat4 GetViewMatrix(const Camera& camera);

// Mesh --------------------------------------------------------------------------------------------

struct Mesh {
    const char* Name = nullptr;
    GLuint VAO = GL_NONE;
};
inline bool IsValid(const Mesh& mesh) { return mesh.VAO != GL_NONE; }

struct CreateMeshOptions {
    std::span<float> Vertices;
    std::span<u32> Indices;
    GLenum MemoryUsage = GL_STATIC_DRAW;
    // Each non-zery entry represents an attrib pointer to set.
    // The value represents amount of elements.
    // Stride is assumed to be contiguous.
    std::array<u8, 4> AttribPointers = {};
    // If non-zero, we will use this value rather than calculated given the |AttribPointers|.
    u8 Stride = 0;
};
Mesh CreateMesh(const char* name, const CreateMeshOptions& options);
void Bind(const Mesh& mesh);

// Shader ------------------------------------------------------------------------------------------

struct Shader {
	const char* Name = nullptr;
    GLuint Program = GL_NONE;
};

inline bool IsValid(const Shader& shader) { return shader.Program != GL_NONE; }

Shader CreateShader(const char* name, const char* vs_path, const char* fs_path);
Shader CreateShaderFromString(const char* name, const char* vs_source, const char* fs_source);

void Use(const Shader& shader);
void SetBool(const Shader& shader, const char* uniform, bool value);
void SetI32(const Shader& shader, const char* uniform, i32 value);
void SetU32(const Shader& shader, const char* uniform, u32 value);
void SetFloat(const Shader& shader, const char* uniform, float value);
void SetVec3(const Shader& shader, const char* uniform, float* value);
void SetMat4(const Shader& shader, const char* uniform, float* value);

// Texture -----------------------------------------------------------------------------------------

struct Texture {
    i32 Width = 0;
    i32 Height = 0;
    GLuint Handle = GL_NONE;
};
bool IsValid(const Texture& texture);

struct LoadTextureOptions {
    bool FlipVertically = false;
};
Texture LoadTexture(const char* path, const LoadTextureOptions& options = {});
void Bind(const Texture& texture, GLuint texture_unit);

}  // namespace kdk
