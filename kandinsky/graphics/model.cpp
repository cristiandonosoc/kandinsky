#include <kandinsky/graphics/model.h>

#include <kandinsky/platform.h>

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/importer.hpp>

namespace kdk {

// MODEL -------------------------------------------------------------------------------------------

namespace opengl_private {

struct CreateModelContext {
    PlatformState* Platform = nullptr;
    CreateModelOptions Options = {};

    String Path = {};
    String Dir = {};

    const aiScene* Scene = nullptr;

    std::array<Mesh*, Model::kMaxMeshes> Meshes = {};
    u32 MeshCount = 0;
};

void ProcessMaterial(Arena* arena,
                     CreateModelContext* model_context,
                     aiMaterial* aimaterial,
                     aiTextureType texture_type,
                     Material* out) {
    auto scratch = GetScratchArena(arena);

    // Material material = {};

    u32 texture_count = aimaterial->GetTextureCount(texture_type);
    for (u32 i = 0; i < texture_count; i++) {
        aiString relative_path;
        aimaterial->GetTexture(texture_type, i, &relative_path);
        String path = paths::PathJoin(scratch.Arena,
                                      model_context->Dir,
                                      String(relative_path.data, relative_path.length));
        String basename = paths::GetBasename(scratch.Arena, path);

        ETextureType tt = ETextureType::None;
        if (texture_type == aiTextureType_DIFFUSE) {
            tt = ETextureType::Diffuse;
        } else if (texture_type == aiTextureType_SPECULAR) {
            tt = ETextureType::Specular;
        } else if (texture_type == aiTextureType_EMISSIVE) {
            tt = ETextureType::Emissive;
        }

        LoadTextureOptions options{
            .Type = tt,
        };

        Texture* texture =
            CreateTexture(&model_context->Platform->Textures, basename.Str(), path.Str(), options);
        if (!texture) {
            SDL_Log("ERROR: Loading texture %s\n", path.Str());
            return;
        }

        ASSERT(out->TextureCount < Material::kMaxTextures);
        out->Textures[out->TextureCount++] = texture;
    }
}

Mesh* ProcessMesh(Arena* arena, CreateModelContext* model_context, aiMesh* aimesh) {
    ASSERT(model_context->MeshCount < Model::kMaxMeshes);

    auto scratch = GetScratchArena(arena);

    String mesh_name =
        Printf(scratch.Arena, "%s_%d", model_context->Path.Str(), model_context->MeshCount);
    if (Mesh* found = FindMesh(&model_context->Platform->Meshes, mesh_name.Str())) {
        return found;
    }

    // We start from the given options.
    CreateMeshOptions mesh_context = model_context->Options.MeshOptions;

    // Process the vertices.
    mesh_context.Vertices = (Vertex*)ArenaPushArray<Vertex>(arena, aimesh->mNumVertices);
    mesh_context.VertexCount = aimesh->mNumVertices;
    Vertex* vertex_ptr = mesh_context.Vertices;

    for (u32 i = 0; i < aimesh->mNumVertices; i++) {
        *vertex_ptr = {};
        std::memcpy(&vertex_ptr->Position, &aimesh->mVertices[i], sizeof(Vec3));
        std::memcpy(&vertex_ptr->Normal, &aimesh->mNormals[i], sizeof(Vec3));
        if (aimesh->mTextureCoords[0]) {
            aiVector3D& uv = aimesh->mTextureCoords[0][i];
            vertex_ptr->UVs.x = uv.x;
            vertex_ptr->UVs.y = uv.y;
        }

        vertex_ptr++;
    }
    ASSERT(vertex_ptr == (mesh_context.Vertices + aimesh->mNumVertices));

    // Process the indices.
    // We make a first pass to know how much to allocate.
    for (u32 i = 0; i < aimesh->mNumFaces; i++) {
        mesh_context.IndexCount += aimesh->mFaces[i].mNumIndices;
    }

    // Now we can collect the indices in one nice array.
    // TODO(cdc): Likely there is a clever way to join the arena allocations.
    mesh_context.Indices = (u32*)ArenaPushArray<u32>(arena, mesh_context.IndexCount);
    u32* index_ptr = mesh_context.Indices;
    for (u32 i = 0; i < aimesh->mNumFaces; i++) {
        const aiFace& face = aimesh->mFaces[i];
        std::memcpy(index_ptr, face.mIndices, face.mNumIndices * sizeof(u32));
        index_ptr += face.mNumIndices;
    }
    ASSERT(index_ptr == (mesh_context.Indices + mesh_context.IndexCount));

    Material out_material = {};
    aiMaterial* aimaterial = model_context->Scene->mMaterials[aimesh->mMaterialIndex];
    ProcessMaterial(arena, model_context, aimaterial, aiTextureType_DIFFUSE, &out_material);
    ProcessMaterial(arena, model_context, aimaterial, aiTextureType_SPECULAR, &out_material);
    ProcessMaterial(arena, model_context, aimaterial, aiTextureType_EMISSIVE, &out_material);

    // TODO(cdc): Deduplicate materials if they are the same by fingerprint.
    mesh_context.Material =
        CreateMaterial(&model_context->Platform->Materials, mesh_name.Str(), out_material);

    // Now that we have everthing loaded, we can create the mesh.
    Mesh* mesh = CreateMesh(&model_context->Platform->Meshes, mesh_name.Str(), mesh_context);
    if (!mesh) {
        SDL_Log("ERROR: Creating mesh %s\n", mesh_name.Str());
        return nullptr;
    }

    return mesh;
}

bool ProcessNode(Arena* arena, CreateModelContext* context, aiNode* node) {
    for (u32 i = 0; i < node->mNumMeshes; i++) {
        aiMesh* aimesh = context->Scene->mMeshes[node->mMeshes[i]];

        Mesh* mesh = ProcessMesh(arena, context, aimesh);
        if (!mesh) {
            SDL_Log("ERROR: ProcessNode");
            context->MeshCount++;
            return false;
        }

        context->Meshes[context->MeshCount++] = mesh;
    }

    for (u32 i = 0; i < node->mNumChildren; i++) {
        ProcessNode(arena, context, node->mChildren[i]);
    }

    return true;
}

}  // namespace opengl_private

Model* CreateModel(Arena* arena,
                   ModelRegistry* registry,
                   String path,
                   const CreateModelOptions& options) {
    using namespace opengl_private;

    u32 id = IDFromString(path.Str());
    if (Model* found = FindModel(registry, id)) {
        return found;
    }

    ASSERT(registry->ModelCount < ModelRegistry::kMaxModels);

    Assimp::Importer importer;

    u32 ai_flags = aiProcess_Triangulate;
    if (options.FlipUVs) {
        ai_flags |= aiProcess_FlipUVs;
    }

    const aiScene* scene = importer.ReadFile(path.Str(), ai_flags);
    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        SDL_Log("ERROR: CreateModel: %s\n", importer.GetErrorString());
        return nullptr;
    }

    SDL_Log("Model %s\n", path.Str());

    auto scratch = GetScratchArena(arena);

    auto* context = ArenaPushZero<CreateModelContext>(scratch.Arena);
    context->Platform = platform::GetPlatformContext();
    context->Options = options;
    context->Path = String(path);
    context->Dir = paths::GetDirname(scratch.Arena, context->Path);
    context->Scene = scene;

    if (!ProcessNode(arena, context, scene->mRootNode)) {
        SDL_Log("ERROR: Processing model %s\n", path.Str());
    }

    Model model{
        .ID = id,
        .Path = platform::InternToStringArena(path.Str()),
    };
    // std::memcpy(model.Meshes.data(), context->Meshes.data(), sizeof(context->Meshes));
    model.Meshes = context->Meshes;
    model.MeshCount = context->MeshCount;

    SDL_Log("Created model %s. Meshes: %u\n", path.Str(), model.MeshCount);

    registry->Models[registry->ModelCount++] = std::move(model);
    return &registry->Models[registry->ModelCount - 1];
}

Model* CreateModelFromMesh(ModelRegistry* registry, String path, Mesh* mesh) {
    u32 id = IDFromString(path.Str());
    if (Model* found = FindModel(registry, id)) {
        ASSERT(false);
        return found;
    }

    ASSERT(registry->ModelCount < ModelRegistry::kMaxModels);

    Model model{
        .ID = id,
        .Path = platform::InternToStringArena(path.Str()),
    };
    model.Meshes[model.MeshCount++] = mesh;

    SDL_Log("Created model %s. Meshes: %u\n", path.Str(), model.MeshCount);

    registry->Models[registry->ModelCount++] = std::move(model);
    return &registry->Models[registry->ModelCount - 1];
}

Model* FindModel(ModelRegistry* registry, u32 id) {
    for (u32 i = 0; i < registry->ModelCount; i++) {
        auto& model = registry->Models[i];
        if (model.ID == id) {
            return &model;
        }
    }

    return nullptr;
}

void Draw(const Model& model, const Shader& shader, const RenderState& rs) {
    for (u32 i = 0; i < model.MeshCount; i++) {
        const Mesh* mesh = model.Meshes[i];
        Draw(*mesh, shader, rs);
    }
}

// COMPONENTS --------------------------------------------------------------------------------------

void OnLoadedOnEntity(Entity* entity, StaticModelComponent* smc) {
    ASSERT(entity);
    ASSERT(smc);
    LoadAssets(smc);
}

void LoadAssets(StaticModelComponent* smc) {
    auto scratch = GetScratchArena();

    if (!smc->ModelPath.IsEmpty()) {
        ModelRegistry* registry = &platform::GetPlatformContext()->Models;
        smc->Model = CreateModel(scratch.Arena, registry, smc->ModelPath);
        if (!smc->Model) {
            SDL_Log("ERROR: Failed to load model %s\n", smc->ModelPath.Str());
        }
    }

    if (!smc->ShaderPath.IsEmpty()) {
        ShaderRegistry* registry = &platform::GetPlatformContext()->Shaders;
        smc->Shader = CreateShader(registry, smc->ShaderPath);
        if (!smc->Shader) {
            SDL_Log("ERROR: Failed to load shader %s\n", smc->ShaderPath.Str());
        }
    }
}

void BuildImGui(StaticModelComponent* smc) {
	ImGui::Text("ModelPath: %s", smc->ModelPath.Str());
	ImGui::Text("ShaderPath: %s", smc->ShaderPath.Str());
}

}  // namespace kdk
