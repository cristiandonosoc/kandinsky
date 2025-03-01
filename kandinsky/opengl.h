#pragma once

#include <kandinsky/color.h>
#include <kandinsky/defines.h>
#include <kandinsky/math.h>
#include <kandinsky/print.h>

#include <SDL3/SDL_stdinc.h>

// clang-format off
// We need this header ordering sadly.
#include <GL/glew.h>
#include <GL/GLU.h>
// clang-format on

#include <span>
#include <string>
#include <vector>

namespace kdk {

struct PlatformState;
struct Shader;
struct Texture;

// Camera ------------------------------------------------------------------------------------------

struct Camera {
    Vec3 Position = {};
    Vec3 Front = {};
    Vec3 Up = {};
    Vec3 Right = {};

    // Euler angles (in radians).
    float Yaw = 0;
    float Pitch = 0;

    float MovementSpeed = 2.5f;
    float MouseSensitivity = 0.1f;

    // Cached Values.
    Mat4 View = Mat4(1.0f);
    Mat4 Proj = Mat4(1.0f);
    Mat4 ViewProj = Mat4(1.0f);
};

// In radians.
void OffsetEulerAngles(Camera* camera, float yaw, float pitch);
void Update(PlatformState* ps, Camera* camera, double dt);

// LineBatcher -------------------------------------------------------------------------------------

struct LineBatch {
    GLenum Mode = GL_LINES;
    Vec4 Color = Vec4(1);
    float LineWidth = 1.0f;
    u32 PrimitiveCount = 0;
};

struct LineBatcher {
    const char* Name = nullptr;
    u32 ID = 0;

    GLuint VAO = GL_NONE;
    GLuint VBO = GL_NONE;

    std::vector<LineBatch> Batches = {};
    std::vector<u8> Data = {};
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
void AddPoint(LineBatcher* lb, const Vec3& point);
void AddPoints(LineBatcher* lb, const Vec3& p1, const Vec3& p2);
void AddPoints(LineBatcher* lb, std::span<const Vec3> points);

void Buffer(PlatformState* ps, const LineBatcher& lb);
void Draw(const LineBatcher& lb, const Shader& shader);

struct LineBatcherRegistry {
    LineBatcher LineBatchers[4] = {};
    u32 Count = 0;
};

LineBatcher* CreateLineBatcher(LineBatcherRegistry* registry, const char* name);
LineBatcher* FindLineBatcher(LineBatcherRegistry* registry, u32 id);
inline LineBatcher* FindLineBatcher(LineBatcherRegistry* registry, const char* name) {
    return FindLineBatcher(registry, IDFromString(name));
}

// Mesh --------------------------------------------------------------------------------------------

struct Vertex {
    Vec3 Position;
    Vec3 Normal;
    Vec2 UVs;
};

struct Mesh {
    const char* Name = nullptr;
    u32 ID = 0;
    GLuint VAO = GL_NONE;

    u32 VertexCount = 0;
    u32 IndexCount = 0;

    Texture* Textures[4] = {};
};
inline bool IsValid(const Mesh& mesh) { return mesh.VAO != GL_NONE; }

void Draw(const Mesh& mesh, const Shader& shader);

struct MeshRegistry {
    Mesh Meshes[32];
    u32 MeshCount = 0;
};

struct CreateMeshOptions {
    Vertex* Vertices = nullptr;
    u32* Indices = nullptr;

    u32 VertexCount = 0;
    u32 IndexCount = 0;

    Texture* Textures[4] = {};

    GLenum MemoryUsage = GL_STATIC_DRAW;

    /*
// Each non-zery entry represents an attrib pointer to set.
// The value represents amount of elements.
// Stride is assumed to be contiguous.
std::array<u8, 4> AttribPointers = {};

// If non-zero, we will use this value rather than calculated given the |AttribPointers|.
u8 Stride = 0;
    */
};
Mesh* CreateMesh(MeshRegistry* registry, const char* name, const CreateMeshOptions& options);
Mesh* FindMesh(MeshRegistry* registry, u32 id);
inline Mesh* FindMesh(MeshRegistry* registry, const char* name) {
    return FindMesh(registry, IDFromString(name));
}

static_assert(sizeof(Mesh::Textures) == sizeof(CreateMeshOptions::Textures));

// Model -------------------------------------------------------------------------------------------

struct Model {
    const char* Name = nullptr;
    Mesh** Meshes = nullptr;
    u32 MeshCount = 0;
};

struct ModelRegistry {
    Model Models[32];
    u32 Count = 0;
};

Model* CreateModel(ModelRegistry*, const char* name, const char* path);

// Shader ------------------------------------------------------------------------------------------

struct Shader {
    const char* Name = nullptr;
    u32 ID = 0;
    std::string VertPath = {};
    std::string FragPath = {};
    SDL_Time LastLoadTime = 0;

    GLuint Program = GL_NONE;
};

inline bool IsValid(const Shader& shader) { return shader.Program != GL_NONE; }

void Use(const Shader& shader);
void SetBool(const Shader& shader, const char* uniform, bool value);
void SetI32(const Shader& shader, const char* uniform, i32 value);
void SetU32(const Shader& shader, const char* uniform, u32 value);
void SetFloat(const Shader& shader, const char* uniform, float value);
void SetVec2(const Shader& shader, const char* uniform, const Vec2& value);
void SetVec3(const Shader& shader, const char* uniform, const Vec3& value);
void SetVec4(const Shader& shader, const char* uniform, const Vec4& value);
void SetMat4(const Shader& shader, const char* uniform, const float* value);

struct ShaderRegistry {
    Shader Shaders[32] = {};
    u32 Count = 0;
};

Shader* CreateShader(ShaderRegistry* registry,
                     const char* name,
                     const char* vert_path,
                     const char* frag_path);
Shader* CreateShaderFromString(ShaderRegistry* registry,
                               const char* name,
                               const char* vert_source,
                               const char* frag_source);

Shader* FindShader(ShaderRegistry* registry, u32 id);
inline Shader* FindShader(ShaderRegistry* registry, const char* name) {
    return FindShader(registry, IDFromString(name));
}

bool ReevaluateShaders(ShaderRegistry* registry);

// Texture -----------------------------------------------------------------------------------------

enum class ETextureType : u8 {
    None,
    Diffuse,
    Specular,
    Emissive,
};

struct Texture {
    const char* Name = nullptr;
    u32 ID = 0;
    i32 Width = 0;
    i32 Height = 0;
    GLuint Handle = GL_NONE;
    ETextureType Type = ETextureType::None;
};
bool IsValid(const Texture& texture);

struct LoadTextureOptions {
    ETextureType Type = ETextureType::None;
    bool FlipVertically = false;
    GLint WrapS = GL_REPEAT;
    GLint WrapT = GL_REPEAT;
};
void Bind(const Texture& texture, GLuint texture_unit);

struct TextureRegistry {
    Texture Textures[32];
    u32 Count = 0;
};
Texture* CreateTexture(TextureRegistry* registry,
                       const char* name,
                       const char* path,
                       const LoadTextureOptions& options = {});
Texture* FindTexture(TextureRegistry* registry, u32 id);
inline Texture* FindTexture(TextureRegistry* registry, const char* name) {
    return FindTexture(registry, IDFromString(name));
}

}  // namespace kdk
