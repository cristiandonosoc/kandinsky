#pragma once

#include <kandinsky/color.h>
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
#include <string>
#include <vector>

namespace kdk {

struct PlatformState;
struct Shader;

std::string ToString(const glm::vec3& vec);

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
void Update(PlatformState* ps, Camera* camera, float dt);
glm::mat4 GetViewMatrix(const Camera& camera);

// LineBatcher -------------------------------------------------------------------------------------

struct LineBatch {
    GLenum Mode = GL_LINES;
    glm::vec4 Color = glm::vec4(1);
    float LineWidth = 1.0f;
    u32 PrimitiveCount = 0;
};

struct LineBatcher {
    const char* Name = nullptr;

    GLuint VAO = GL_NONE;
    GLuint VBO = GL_NONE;

    std::vector<LineBatch> Batches;
    std::vector<u8> Data;
    i32 CurrentBatch = NONE;
};
inline bool IsValid(const LineBatcher& lb) { return lb.VAO != GL_NONE && lb.VBO != GL_NONE; }

void Reset(LineBatcher* lb);

void StartLineBatch(LineBatcher* lb,
                    GLenum mode = GL_LINES,
                    Color32 color = Color32::White,
                    float line_width = 1.0f);
void EndLineBatch(LineBatcher* lb);

// Must be called between StartLineBatch/EndLineBatch calls.
void AddPoint(LineBatcher* lb, const glm::vec3& point);
void AddPoints(LineBatcher* lb, const glm::vec3& p1, const glm::vec3& p2);
void AddPoints(LineBatcher* lb, std::span<const glm::vec3> points);

void Buffer(PlatformState* ps, const LineBatcher& lb);
void Draw(PlatformState* ps, const Shader& shader, const LineBatcher& lb);

struct LineBatcherRegistry {
    LineBatcher LineBatchers[4] = {};
    u32 Count = 0;
};

LineBatcher* CreateLineBatcher(PlatformState* ps, LineBatcherRegistry* registry, const char* name);
LineBatcher* FindLineBatcher(LineBatcherRegistry* registry, const char* name);

// Mesh --------------------------------------------------------------------------------------------

struct Mesh {
    const char* Name = nullptr;
    GLuint VAO = GL_NONE;
};
inline bool IsValid(const Mesh& mesh) { return mesh.VAO != GL_NONE; }

struct CreateMeshOptions {
    std::span<float> Vertices = {};
    std::span<u32> Indices = {};
    GLenum MemoryUsage = GL_STATIC_DRAW;
    // Each non-zery entry represents an attrib pointer to set.
    // The value represents amount of elements.
    // Stride is assumed to be contiguous.
    std::array<u8, 4> AttribPointers = {};
    // If non-zero, we will use this value rather than calculated given the |AttribPointers|.
    u8 Stride = 0;
};
void Bind(PlatformState* ps, const Mesh& mesh);

struct MeshRegistry {
    Mesh Meshes[32];
    u32 Count = 0;
};

Mesh* CreateMesh(PlatformState* ps,
                 MeshRegistry* registry,
                 const char* name,
                 const CreateMeshOptions& options);
Mesh* FindMesh(MeshRegistry* registry, const char* name);

// Shader ------------------------------------------------------------------------------------------

struct Shader {
    const char* Name = nullptr;
    GLuint Program = GL_NONE;
};

inline bool IsValid(const Shader& shader) { return shader.Program != GL_NONE; }

void Use(PlatformState* ps, const Shader& shader);
void SetBool(PlatformState* ps, const Shader& shader, const char* uniform, bool value);
void SetI32(PlatformState* ps, const Shader& shader, const char* uniform, i32 value);
void SetU32(PlatformState* ps, const Shader& shader, const char* uniform, u32 value);
void SetFloat(PlatformState* ps, const Shader& shader, const char* uniform, float value);
void SetVec3(PlatformState* ps, const Shader& shader, const char* uniform, const glm::vec3& value);
void SetVec4(PlatformState* ps, const Shader& shader, const char* uniform, const glm::vec4& value);
void SetMat4(PlatformState* ps, const Shader& shader, const char* uniform, const float* value);

struct ShaderRegistry {
    Shader Shaders[32] = {};
    u32 Count = 0;
};

Shader* CreateShader(PlatformState* ps,
                     ShaderRegistry* registry,
                     const char* name,
                     const char* vs_path,
                     const char* fs_path);
Shader* CreateShaderFromString(PlatformState* ps,
                               ShaderRegistry* registry,
                               const char* name,
                               const char* vs_source,
                               const char* fs_source);
Shader* FindShader(ShaderRegistry* registry, const char* name);

// Texture -----------------------------------------------------------------------------------------

struct Texture {
    const char* Name = nullptr;
    i32 Width = 0;
    i32 Height = 0;
    GLuint Handle = GL_NONE;
};
bool IsValid(const Texture& texture);

struct LoadTextureOptions {
    bool FlipVertically = false;
    GLint WrapS = GL_REPEAT;
    GLint WrapT = GL_REPEAT;
};
void Bind(PlatformState* ps, const Texture& texture, GLuint texture_unit);

struct TextureRegistry {
    Texture Textures[32];
    u32 Count = 0;
};
Texture* CreateTexture(PlatformState* ps,
                       TextureRegistry* registry,
                       const char* name,
                       const char* path,
                       const LoadTextureOptions& options = {});
Texture* FindTexture(TextureRegistry* registry, const char* name);

}  // namespace kdk
