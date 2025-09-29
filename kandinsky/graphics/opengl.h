#pragma once

#include <kandinsky/asset.h>
#include <kandinsky/core/color.h>
#include <kandinsky/core/container.h>
#include <kandinsky/core/defines.h>
#include <kandinsky/core/math.h>
#include <kandinsky/core/string.h>
#include <kandinsky/entity.h>

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

    Vec3 Albedo = Vec3(0);
    Vec3 Diffuse = Vec3(0);
    float Shininess = 32.0f;
    FixedVector<TextureAssetHandle, kMaxTextures> TextureHandles = {};
};
inline bool IsValid(const Material& material) { return material.ID != NONE; }

struct CreateMaterialParams {
    GENERATE_ASSET_PARAMS();

    FixedVector<TextureAssetHandle, Material::kMaxTextures> TextureHandles = {};
    Vec3 Albedo = Vec3(0);
    Vec3 Diffuse = Vec3(0);
    float Shininess = 32.0f;
};
void Serialize(SerdeArchive* sa, CreateMaterialParams* params);
MaterialAssetHandle CreateMaterial(AssetRegistry* assets,
                                   String asset_path,
                                   const CreateMaterialParams& params);

// Billboard ---------------------------------------------------------------------------------------

struct BillboardComponent {
    GENERATE_COMPONENT(Billboard);

    Vec2 Size = {1, 1};
    TextureAssetHandle TextureHandle = {};
};
inline void Serialize(SerdeArchive*, BillboardComponent*) { ASSERT(false); }
void BuildImGui(BillboardComponent* bc);

void DrawBillboard(PlatformState* ps,
                   const Shader& shader,
                   const Entity& entity,
                   const BillboardComponent& billboard);

}  // namespace kdk
