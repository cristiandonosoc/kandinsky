#pragma once

#include <kandinsky/asset.h>
#include <kandinsky/core/color.h>
#include <kandinsky/core/container.h>
#include <kandinsky/core/defines.h>
#include <kandinsky/core/math.h>
#include <kandinsky/core/string.h>

#include <SDL3/SDL_stdinc.h>

#include <GL/glew.h>

#include <array>
#include <span>
#include <string>
#include <vector>

namespace kdk {

using AssetID = i32;

struct PlatformState;
struct Shader;
struct Texture;
struct RenderState;
struct Mesh;
struct Model;
struct Material;

// Grid --------------------------------------------------------------------------------------------

void DrawGrid(const RenderState& rs, float near = 15.0f, float far = 50.0f);

// LineBatcher -------------------------------------------------------------------------------------

struct LineBatch {
    GLenum Mode = GL_LINES;
    Vec4 Color = Vec4(1);
    float LineWidth = 1.0f;
    i32 PrimitiveCount = 0;
};

struct LineBatcher {
    String Name = {};
    i32 ID = NONE;

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
    static constexpr i32 kMaxLineBatchers = 4;
    LineBatcher LineBatchers[kMaxLineBatchers] = {};
    i32 LineBatcherCount = 0;
};

LineBatcher* CreateLineBatcher(LineBatcherRegistry* registry, const char* name);
LineBatcher* FindLineBatcher(LineBatcherRegistry* registry, i32 id);
inline LineBatcher* FindLineBatcher(LineBatcherRegistry* registry, const char* name) {
    return FindLineBatcher(registry, IDFromString(name));
}

// Material ----------------------------------------------------------------------------------------

struct Material {
    GENERATE_ASSET(Material);

    static constexpr i32 kMaxTextures = 8;

    i32 ID = NONE;
    FixedArray<TextureAssetHandle, kMaxTextures> TextureHandles = {};

    Vec3 Albedo = Vec3(0);
    Vec3 Diffuse = Vec3(0);
    float Shininess = 32.0f;
};
inline bool IsValid(const Material& material) { return material.ID != NONE; }

struct CreateMaterialOptions {
    FixedArray<TextureAssetHandle, Material::kMaxTextures> TextureHandles = {};
    Vec3 Albedo = Vec3(0);
    Vec3 Diffuse = Vec3(0);
    float Shininess = 32.0f;
};
MaterialAssetHandle CreateMaterial(AssetRegistry* assets,
                                   String asset_path,
                                   const CreateMaterialOptions& options);

// Shader ------------------------------------------------------------------------------------------

struct Shader {
    i32 ID = NONE;
    String Path = {};

    SDL_Time LastLoadTime = 0;

    GLuint Program = GL_NONE;
};

inline bool IsValid(const Shader& shader) { return shader.Program != GL_NONE; }

void Use(const Shader& shader);
void SetBool(const Shader& shader, const char* uniform, bool value);
void SetI32(const Shader& shader, const char* uniform, i32 value);
void SetU32(const Shader& shader, const char* uniform, u32 value);
void SetFloat(const Shader& shader, const char* uniform, float value);
void SetUVec2(const Shader& shader, const char* uniform, const UVec2& value);
void SetUVec3(const Shader& shader, const char* uniform, const UVec3& value);
void SetUVec4(const Shader& shader, const char* uniform, const UVec4& value);
void SetVec2(const Shader& shader, const char* uniform, const Vec2& value);
void SetVec3(const Shader& shader, const char* uniform, const Vec3& value);
void SetVec4(const Shader& shader, const char* uniform, const Vec4& value);
void SetMat4(const Shader& shader, const char* uniform, const float* value);

struct ShaderRegistry {
    static constexpr i32 kMaxShaders = 64;
    Shader Shaders[kMaxShaders] = {};
    i32 ShaderCount = 0;
};

Shader* CreateShader(ShaderRegistry* registry, String path);

Shader* FindShader(ShaderRegistry* registry, i32 id);
inline Shader* FindShader(ShaderRegistry* registry, const char* name) {
    return FindShader(registry, IDFromString(name));
}

bool ReevaluateShaders(ShaderRegistry* registry);

}  // namespace kdk
