#pragma once

#include <kandinsky/entity.h>
#include <kandinsky/graphics/opengl.h>

namespace kdk {

struct Model {
    static constexpr u32 kMaxMeshes = 128;

    u32 ID = 0;
    String Path = {};
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
                   String path,
                   const CreateModelOptions& options = {});
Model* CreateModelFromMesh(ModelRegistry* registry, String path, Mesh* mesh);
Model* FindModel(ModelRegistry* registry, u32 id);
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
void LoadAssets(StaticModelComponent* smc);

}  // namespace kdk
