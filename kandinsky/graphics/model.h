#pragma once

#include <kandinsky/asset.h>
#include <kandinsky/entity.h>
#include <kandinsky/graphics/opengl.h>

namespace kdk {

struct AssetRegistry;

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

void Draw(AssetRegistry* registry,
          MeshAssetHandle mesh_handle,
          const Shader& shader,
          const Material& material,
          const RenderState& rs);

struct MeshRegistry {
    static constexpr i32 kMaxMeshes = 1024;
    std::array<Mesh, kMaxMeshes> Meshes = {};
    i32 MeshCount = 0;
};

struct CreateMeshOptions {
    std::span<Vertex> Vertices = {};
    std::span<u32> Indices = {};

    GLenum MemoryUsage = GL_STATIC_DRAW;
};

MeshAssetHandle FindMesh(AssetRegistry* registry, String asset_path);
MeshAssetHandle CreateMesh(AssetRegistry* registry,
                           String asset_path,
                           const CreateMeshOptions& options);
Mesh CreateMeshAsset(String name, const CreateMeshOptions& options);
Mesh* FindMesh(MeshRegistry* registry, i32 id);

// Model -------------------------------------------------------------------------------------------

// Defines how a mesh is bound within a particular model.
struct ModelMeshBinding {
    MeshAssetHandle Mesh = {};
    Material* Material = nullptr;
};
inline bool IsValid(const ModelMeshBinding& mmb) {
    return IsValid(mmb.Mesh) && mmb.Material != nullptr;
}

struct Model {
    GENERATE_ASSET(Model);

    static constexpr i32 kMaxMeshes = 128;

    i32 ID = NONE;
    String Path = {};
    FixedArray<ModelMeshBinding, kMaxMeshes> MeshBindings = {};
};

struct ModelRegistry {
    static constexpr i32 kMaxModels = 64;
    std::array<Model, kMaxModels> Models;
    i32 ModelCount = 0;
};

void Draw(AssetRegistry* registry, const Model& model, const Shader& shader, const RenderState& rs);

struct CreateModelOptions {
    CreateMeshOptions MeshOptions = {};

    bool FlipUVs = false;
};

Model* CreateModel(Arena* arena,
                   ModelRegistry*,
                   String path,
                   const CreateModelOptions& options = {});
Model* CreateModelFromMesh(ModelRegistry* registry, String path, const ModelMeshBinding& mmb);
Model* FindModel(ModelRegistry* registry, i32 id);
inline Model* FindModel(ModelRegistry* registry, const char* name) {
    return FindModel(registry, IDFromString(name));
}

// COMPONENTS --------------------------------------------------------------------------------------

struct StaticModelComponent {
    GENERATE_COMPONENT(StaticModel);

    String ModelPath;
    String ShaderPath;

    Shader* Shader = nullptr;
    Model* Model = nullptr;
};

void OnLoadedOnEntity(Entity* entity, StaticModelComponent* smc);
void Serialize(SerdeArchive* sa, StaticModelComponent* smc);
void LoadAssets(StaticModelComponent* smc);
void BuildImGui(StaticModelComponent* smc);

}  // namespace kdk
