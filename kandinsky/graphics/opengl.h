#pragma once

#include <kandinsky/color.h>
#include <kandinsky/defines.h>
#include <kandinsky/math.h>
#include <kandinsky/print.h>
#include <kandinsky/string.h>

#include <SDL3/SDL_stdinc.h>

// clang-format off
// We need this header ordering sadly.
#include <GL/glew.h>
// clang-format on

#include <array>
#include <span>
#include <string>
#include <vector>

namespace kdk {

struct PlatformState;
struct Shader;
struct Texture;
struct RenderState;

// Base --------------------------------------------------------------------------------------------

bool LoadBaseAssets(PlatformState* ps);

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

// Grid --------------------------------------------------------------------------------------------

void DrawGrid(const RenderState& rs);

// LineBatcher -------------------------------------------------------------------------------------

struct LineBatch {
    GLenum Mode = GL_LINES;
    Vec4 Color = Vec4(1);
    float LineWidth = 1.0f;
    u32 PrimitiveCount = 0;
};

struct LineBatcher {
    String Name = {};
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
    static constexpr u32 kMaxLineBatchers = 4;
    LineBatcher LineBatchers[kMaxLineBatchers] = {};
    u32 LineBatcherCount = 0;
};

LineBatcher* CreateLineBatcher(LineBatcherRegistry* registry, const char* name);
LineBatcher* FindLineBatcher(LineBatcherRegistry* registry, u32 id);
inline LineBatcher* FindLineBatcher(LineBatcherRegistry* registry, const char* name) {
    return FindLineBatcher(registry, IDFromString(name));
}

// Material ----------------------------------------------------------------------------------------

struct Material {
    static constexpr u32 kMaxTextures = 8;

    u32 ID = 0;
    u32 TextureCount = 0;
    std::array<Texture*, kMaxTextures> Textures = {};

    Vec3 Albedo = Vec3(1.0f);
    Vec3 Diffuse = Vec3(1.0f);
};

struct MaterialRegistry {
    static constexpr u32 kMaxMaterials = 1024;
    std::array<Material, kMaxMaterials> Materials = {};
    u32 MaterialCount = 0;
};

Material* CreateMaterial(MaterialRegistry* registry, const char* name, const Material& material);
Material* FindMaterial(MaterialRegistry* registry, u32 id);
inline Material* FindMaterial(MaterialRegistry* registry, const char* name) {
    return FindMaterial(registry, IDFromString(name));
}

// Mesh --------------------------------------------------------------------------------------------

struct Vertex {
    Vec3 Position;
    Vec3 Normal;
    Vec2 UVs;
};

struct Mesh {
    String Name = {};
    u32 ID = 0;
    GLuint VAO = GL_NONE;

    u32 VertexCount = 0;
    u32 IndexCount = 0;

    Material* Material = nullptr;
};
inline bool IsValid(const Mesh& mesh) { return mesh.VAO != GL_NONE; }

void Draw(const Mesh& mesh,
          const Shader& shader,
          const RenderState& rs,
          const Material* override_material = nullptr);

struct MeshRegistry {
    static constexpr u32 kMaxMeshes = 1024;
    std::array<Mesh, kMaxMeshes> Meshes = {};
    u32 MeshCount = 0;
};

struct CreateMeshOptions {
    Vertex* Vertices = nullptr;
    u32* Indices = nullptr;

    u32 VertexCount = 0;
    u32 IndexCount = 0;

    Material* Material = nullptr;

    GLenum MemoryUsage = GL_STATIC_DRAW;
};
Mesh* CreateMesh(MeshRegistry* registry, const char* name, const CreateMeshOptions& options);
Mesh* FindMesh(MeshRegistry* registry, u32 id);
inline Mesh* FindMesh(MeshRegistry* registry, const char* name) {
    return FindMesh(registry, IDFromString(name));
}

// Model -------------------------------------------------------------------------------------------

struct Model {
    static constexpr u32 kMaxMeshes = 128;

    String Name = {};
    String Path = {};
    u32 ID = 0;
    std::array<Mesh*, kMaxMeshes> Meshes = {};
    u32 MeshCount = 0;
};

struct ModelRegistry {
    static constexpr u32 kMaxModels = 64;
    std::array<Model, kMaxModels> Models;
    u32 ModelCount = 0;
};

void Draw(const Model& model, const Shader& shader, const RenderState& rs);

struct CreateModelOptions {
    CreateMeshOptions MeshOptions = {};

    bool FlipUVs = false;
};

Model* CreateModel(Arena* arena,
                   ModelRegistry*,
                   const char* name,
                   const char* path,
                   const CreateModelOptions& options = {});
Model* FindModel(ModelRegistry* registry, u32 id);
inline Model* FindModel(ModelRegistry* registry, const char* name) {
    return FindModel(registry, IDFromString(name));
}

// Shader ------------------------------------------------------------------------------------------

struct Shader {
    String Name = {};
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
    static constexpr u32 kMaxShaders = 64;
    Shader Shaders[kMaxShaders] = {};
    u32 ShaderCount = 0;
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
    String Name = {};
    String Path = {};
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
    static constexpr u32 kMaxTextures = 64;
    Texture Textures[kMaxTextures];
    u32 TextureCount = 0;
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
