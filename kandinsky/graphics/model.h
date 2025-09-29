#pragma once

#include <kandinsky/asset.h>
#include <kandinsky/entity.h>
#include <kandinsky/graphics/opengl.h>

namespace kdk {

// Mesh --------------------------------------------------------------------------------------------

struct Vertex {
    Vec3 Position;
    Vec3 Normal;
    Vec2 UVs;
};

// Mesh describes the basic resources of a mesh: vertices and indices.
// Texture and material description are normally bound at the model level.
struct Mesh {
    GENERATE_ASSET(Mesh);

    String Name = {};
    i32 ID = NONE;
    GLuint VAO = GL_NONE;

    i32 VertexCount = 0;
    i32 IndexCount = 0;
};
inline bool IsValid(const Mesh& mesh) { return mesh.VAO != GL_NONE; }

void Draw(AssetRegistry* assets,
          MeshAssetHandle mesh_handle,
          ShaderAssetHandle shader_handle,
          const Material& material,
          const RenderState& rs);

struct CreateMeshParams {
    GENERATE_ASSET_PARAMS();

    std::span<Vertex> Vertices = {};
    std::span<u32> Indices = {};

    GLenum MemoryUsage = GL_STATIC_DRAW;
};
inline void Serialize(SerdeArchive*, CreateMeshParams*) {}  // For now mesh don't do anything.

MeshAssetHandle CreateMesh(AssetRegistry* assets,
                           String asset_path,
                           const CreateMeshParams& params);

// Model -------------------------------------------------------------------------------------------

// Defines how a mesh is bound within a particular model.
struct ModelMeshBinding {
    MeshAssetHandle MeshHandle = {};
    MaterialAssetHandle MaterialHandle = {};
};
inline bool IsValid(const ModelMeshBinding& mmb) {
    return IsValid(mmb.MeshHandle) && IsValid(mmb.MaterialHandle);
}

struct Model {
    GENERATE_ASSET(Model);

    static constexpr i32 kMaxMeshes = 128;

    i32 ID = NONE;
    String Path = {};
    FixedVector<ModelMeshBinding, kMaxMeshes> MeshBindings = {};
};

struct ModelRegistry {
    static constexpr i32 kMaxModels = 64;
    Array<Model, kMaxModels> Models;
    i32 ModelCount = 0;
};

void Draw(AssetRegistry* assets,
          ModelAssetHandle model_handle,
          ShaderAssetHandle shader_handle,
          const RenderState& rs);

struct CreateModelParams {
    GENERATE_ASSET_PARAMS();

    bool FlipUVs = false;
    CreateMeshParams MeshOptions = {};
};
void Serialize(SerdeArchive* sa, CreateModelParams* params);

ModelAssetHandle CreateModel(AssetRegistry* assets,
                             String path,
                             const CreateModelParams& params = {});
ModelAssetHandle CreateSyntheticModel(AssetRegistry* assets,
                                      String path,
                                      std::span<const ModelMeshBinding> mmb,
                                      const CreateModelParams& params = {});

// COMPONENTS --------------------------------------------------------------------------------------

struct StaticModelComponent {
    GENERATE_COMPONENT(StaticModel);

    String ModelPath;
    String ShaderPath;

    ModelAssetHandle ModelHandle = {};
    ShaderAssetHandle ShaderHandle = {};
};

void OnLoadedOnEntity(Entity* entity, StaticModelComponent* smc);
void Serialize(SerdeArchive* sa, StaticModelComponent* smc);
void LoadAssets(StaticModelComponent* smc);
void BuildImGui(StaticModelComponent* smc);

}  // namespace kdk
