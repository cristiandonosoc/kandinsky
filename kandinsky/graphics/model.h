#pragma once

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
    String Name = {};
    i32 ID = NONE;
    GLuint VAO = GL_NONE;

    u32 VertexCount = 0;
    u32 IndexCount = 0;
};
inline bool IsValid(const Mesh& mesh) { return mesh.VAO != GL_NONE; }

void Draw(const Mesh& mesh, const Shader& shader, const Material& material, const RenderState& rs);

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

    GLenum MemoryUsage = GL_STATIC_DRAW;
};
Mesh* CreateMesh(MeshRegistry* registry, const char* name, const CreateMeshOptions& options);
Mesh* FindMesh(MeshRegistry* registry, i32 id);
inline Mesh* FindMesh(MeshRegistry* registry, const char* name) {
    return FindMesh(registry, IDFromString(name));
}

// Model -------------------------------------------------------------------------------------------

// Defines how a mesh is bound within a particular model.
struct ModelMeshBinding {
    Mesh* Mesh = nullptr;
    Material* Material = nullptr;
};
inline bool IsValid(const ModelMeshBinding& mmb) {
    return mmb.Mesh != nullptr && mmb.Material != nullptr;
}

struct Model {
    static constexpr u32 kMaxMeshes = 128;

    i32 ID = NONE;
    String Path = {};
    FixedArray<ModelMeshBinding, kMaxMeshes> MeshBindings = {};
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
