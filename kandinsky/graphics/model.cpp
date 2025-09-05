#include <kandinsky/graphics/model.h>

#include <kandinsky/asset_registry.h>
#include <kandinsky/graphics/opengl.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/platform.h>

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/importer.hpp>

namespace kdk {

// MESH --------------------------------------------------------------------------------------------

namespace opengl_private {

std::array kDiffuseSamplerNames{
    "uMaterial.TextureDiffuse1",
    "uMaterial.TextureDiffuse2",
    "uMaterial.TextureDiffuse3",
};

std::array kSpecularSamplerNames{
    "uMaterial.TextureSpecular1",
    "uMaterial.TextureSpecular2",
};

std::array kEmissiveSamplerNames{
    "uMaterial.TextureEmissive1",
};

}  // namespace opengl_private

void Draw(AssetRegistry* assets,
          MeshAssetHandle mesh_handle,
          ShaderAssetHandle shader_handle,
          MaterialAssetHandle material_handle,
          const RenderState& rs) {
    using namespace opengl_private;

    auto [_m, mesh] = FindMeshAsset(assets, mesh_handle);
    ASSERT(mesh);

    auto [_s, shader] = FindShaderAsset(assets, shader_handle);
    ASSERT(shader);
    ASSERT(IsValid(*shader));

    u32 diffuse_index = 0;
    u32 specular_index = 0;
    u32 emissive_index = 0;

    Use(*shader);

    SetUniforms(rs, *shader);

    // Setup the textures.
    // const Material* material = override_material ? override_material : mesh.Material;
    if (IsValid(material_handle)) {
        auto [_ma, material] = FindMaterialAsset(assets, material_handle);
        ASSERT(material);

        SetVec3(*shader, "uMaterial.Albedo", material->Albedo);
        SetVec3(*shader, "uMaterial.Diffuse", material->Diffuse);
        SetFloat(*shader, "uMaterial.Shininess", material->Shininess);

        for (i32 texture_index = 0; texture_index < Material::kMaxTextures; texture_index++) {
            // If we don't have this index, we bind it the zero.
            if (material->TextureHandles.Size <= texture_index) {
                glActiveTexture(GL_TEXTURE0 + texture_index);
                glBindTexture(GL_TEXTURE_2D, NULL);
                continue;
            }

            TextureAssetHandle texture_handle = material->TextureHandles[texture_index];
            ASSERT(IsValid(texture_handle));

            auto [_ta, texture] = FindTextureAsset(assets, texture_handle);
            ASSERT(texture);

            glActiveTexture(GL_TEXTURE0 + texture_index);
            glBindTexture(GL_TEXTURE_2D, texture->Handle);

            switch (texture->Type) {
                case ETextureType::None: continue;
                case ETextureType::Diffuse: {
                    ASSERT(diffuse_index < kDiffuseSamplerNames.size());
                    SetI32(*shader, kDiffuseSamplerNames[diffuse_index], texture_index);
                    diffuse_index++;
                    break;
                }
                case ETextureType::Specular: {
                    ASSERT(specular_index < kSpecularSamplerNames.size());
                    SetI32(*shader, kSpecularSamplerNames[specular_index], texture_index);
                    specular_index++;
                    break;
                }
                case ETextureType::Emissive: {
                    ASSERT(emissive_index < kEmissiveSamplerNames.size());
                    SetI32(*shader, kEmissiveSamplerNames[emissive_index], texture_index);
                    emissive_index++;
                    break;
                }
            }
        }

    } else {
        SetVec3(*shader, "uMaterial.Albedo", Vec3(0.1f));
        SetVec3(*shader, "uMaterial.Diffuse", Vec3(0.1f));
        SetFloat(*shader, "uMaterial.Shininess", 0);

        // Unbind all textures.
        for (u32 texture_index = 0; texture_index < Material::kMaxTextures; texture_index++) {
            glActiveTexture(GL_TEXTURE0 + texture_index);
            glBindTexture(GL_TEXTURE_2D, NULL);
        }
    }

    // Make the draw call.
    glBindVertexArray(mesh->VAO);
    if (mesh->IndexCount == 0) {
        glDrawArrays(GL_TRIANGLES, 0, mesh->VertexCount);
    } else {
        glDrawElements(GL_TRIANGLES, mesh->IndexCount, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(NULL);
}

MeshAssetHandle CreateMesh(AssetRegistry* assets,
                           String asset_path,
                           const CreateMeshParams& params) {
    // Check if the asset exists already.
    i32 asset_id = GenerateAssetID(EAssetType::Mesh, asset_path);
    if (AssetHandle handle = assets->MeshHolder.FindAssetHandle(asset_id); IsValid(handle)) {
        return {handle};
    }

    if (params.Vertices.empty()) {
        SDL_Log("ERROR: Creating Mesh %s: no indices provided", asset_path.Str());
        return {};
    }

    GLuint vao = GL_NONE;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Copy our vertices into a Vertex Buffer Object (VBO).
    GLuint vbo = GL_NONE;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 params.Vertices.size_bytes(),
                 params.Vertices.data(),
                 params.MemoryUsage);

    if (!params.Indices.empty()) {
        GLuint ebo = GL_NONE;
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     params.Indices.size_bytes(),
                     params.Indices.data(),
                     params.MemoryUsage);
    }

    GLsizei stride = sizeof(Vertex);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, UVs));
    glEnableVertexAttribArray(2);

    glBindVertexArray(GL_NONE);

    Mesh mesh{
        .Name = platform::InternToStringArena(asset_path.Str()),
        .ID = asset_id,
        .VAO = vao,
        .VertexCount = (i32)params.Vertices.size(),
        .IndexCount = (i32)params.Indices.size(),
    };

    SDL_Log("Created mesh %s. Vertices %u, Indices: %u\n",
            asset_path.Str(),
            mesh.VertexCount,
            mesh.IndexCount);

    AssetHandle result =
        assets->MeshHolder.PushAsset(asset_id, asset_path, params.AssetOptions, std::move(mesh));
    return {result};
}

// MODEL -------------------------------------------------------------------------------------------

void Serialize(SerdeArchive* sa, CreateModelParams* params) { SERDE(sa, params, FlipUVs); }

namespace opengl_private {

struct CreateModelContext {
    PlatformState* Platform = nullptr;
    AssetRegistry* Assets = nullptr;
    CreateModelParams Options = {};

    String AssetPath = {};
    String FullPath = {};
    String Dir = {};

    const aiScene* Scene = nullptr;

    u32 ProcessedMeshCount = 0;
    FixedArray<ModelMeshBinding, Model::kMaxMeshes> MeshBindings = {};
};

void ProcessMaterial(Arena* arena,
                     CreateModelContext* model_context,
                     aiMaterial* aimaterial,
                     aiTextureType texture_type,
                     CreateMaterialParams* out) {
    auto scratch = GetScratchArena(arena);

    // Material material = {};

    u32 texture_count = aimaterial->GetTextureCount(texture_type);
    for (u32 i = 0; i < texture_count; i++) {
        aiString relative_path;
        aimaterial->GetTexture(texture_type, i, &relative_path);
        String path = paths::PathJoin(scratch.Arena,
                                      model_context->Dir,
                                      String(relative_path.data, relative_path.length));
        path = RemovePrefix(scratch, path, model_context->Assets->AssetBasePath);

        ETextureType tt = ETextureType::None;
        if (texture_type == aiTextureType_DIFFUSE) {
            tt = ETextureType::Diffuse;
        } else if (texture_type == aiTextureType_SPECULAR) {
            tt = ETextureType::Specular;
        } else if (texture_type == aiTextureType_EMISSIVE) {
            tt = ETextureType::Emissive;
        } else {
            // Unsupported texture type.
            continue;
        }

        CreateTextureParams texture_params{
            .Type = tt,
        };
        TextureAssetHandle texture_handle =
            CreateTexture(model_context->Assets, path, texture_params);
        if (!IsValid(texture_handle)) {
            SDL_Log("ERROR: Loading texture %s\n", path.Str());
            return;
        }

        out->TextureHandles.Push(texture_handle);
    }
}

ModelMeshBinding ProcessMesh(Arena* arena, CreateModelContext* model_context, aiMesh* aimesh) {
    ASSERT(model_context->MeshBindings.Size < Model::kMaxMeshes);

    auto scratch = GetScratchArena(arena);

    MeshAssetHandle mesh = {};

    String mesh_name = Printf(scratch.Arena,
                              "%s_%d",
                              model_context->AssetPath.Str(),
                              model_context->ProcessedMeshCount);
    if (MeshAssetHandle found = FindMeshHandle(model_context->Assets, mesh_name); IsValid(found)) {
        mesh = found;
    } else {
        // We start from the given options.
        CreateMeshParams mesh_context = model_context->Options.MeshOptions;

        // Process the vertices.
        mesh_context.Vertices = ArenaPushArray<Vertex>(arena, aimesh->mNumVertices);
        Vertex* vertex_ptr = mesh_context.Vertices.data();

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
        ASSERT(vertex_ptr == (mesh_context.Vertices.data() + mesh_context.Vertices.size()));

        // Process the indices.
        // We make a first pass to know how much to allocate.
        int index_count = 0;
        for (u32 i = 0; i < aimesh->mNumFaces; i++) {
            index_count += aimesh->mFaces[i].mNumIndices;
        }

        // Now we can collect the indices in one nice array.
        // TODO(cdc): Likely there is a clever way to join the arena allocations.
        mesh_context.Indices = ArenaPushArray<u32>(arena, index_count);
        u32* index_ptr = mesh_context.Indices.data();
        for (u32 i = 0; i < aimesh->mNumFaces; i++) {
            const aiFace& face = aimesh->mFaces[i];
            std::memcpy(index_ptr, face.mIndices, face.mNumIndices * sizeof(u32));
            index_ptr += face.mNumIndices;
        }
        ASSERT(index_ptr == (mesh_context.Indices.data() + mesh_context.Indices.size()));

        // Now that we have everthing loaded, we can create the mesh.
        MeshAssetHandle created = CreateMesh(model_context->Assets, mesh_name, mesh_context);
        if (!IsValid(created)) {
            SDL_Log("ERROR: Creating mesh %s\n", mesh_name.Str());
            return {};
        }
        mesh = created;
    }

    // Create the material.
    // TODO(cdc): Deduplicate materials if they are the same by fingerprint.
    //            Currently we will duplicate a lot of materials.
    CreateMaterialParams material_options = {};
    aiMaterial* aimaterial = model_context->Scene->mMaterials[aimesh->mMaterialIndex];
    ProcessMaterial(arena, model_context, aimaterial, aiTextureType_DIFFUSE, &material_options);
    ProcessMaterial(arena, model_context, aimaterial, aiTextureType_SPECULAR, &material_options);
    ProcessMaterial(arena, model_context, aimaterial, aiTextureType_EMISSIVE, &material_options);

    MaterialAssetHandle material_handle =
        CreateMaterial(model_context->Assets, mesh_name, material_options);

    return ModelMeshBinding{
        .MeshHandle = mesh,
        .MaterialHandle = material_handle,
    };
}

bool ProcessNode(Arena* arena, CreateModelContext* context, aiNode* node) {
    for (u32 i = 0; i < node->mNumMeshes; i++) {
        aiMesh* aimesh = context->Scene->mMeshes[node->mMeshes[i]];

        ModelMeshBinding mmb = ProcessMesh(arena, context, aimesh);
        context->ProcessedMeshCount++;
        if (!IsValid(mmb)) {
            SDL_Log("ERROR: ProcessNode");
            return false;
        }

        context->MeshBindings.Push(mmb);
    }

    for (u32 i = 0; i < node->mNumChildren; i++) {
        ProcessNode(arena, context, node->mChildren[i]);
    }

    return true;
}

}  // namespace opengl_private

ModelAssetHandle CreateModel(AssetRegistry* assets,
                             String asset_path,
                             const CreateModelParams& params) {
    using namespace opengl_private;

    if (ModelAssetHandle found = FindModelHandle(assets, asset_path); IsValid(found)) {
        return found;
    }

    ASSERT(!assets->ModelHolder.IsFull());

    auto scoped_arena = assets->AssetLoadingArena->GetScopedArena();
    auto scratch = GetScratchArena();

    Assimp::Importer importer;
    u32 ai_flags = aiProcess_Triangulate;
    if (params.FlipUVs) {
        ai_flags |= aiProcess_FlipUVs;
    }

    String full_asset_path = GetFullAssetPath(scratch, assets, asset_path);

    const aiScene* scene = importer.ReadFile(full_asset_path.Str(), ai_flags);
    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        SDL_Log("ERROR: CreateModel: %s\n", importer.GetErrorString());
        return {};
    }

    SDL_Log("Model %s\n", asset_path.Str());

    auto* context = ArenaPushZero<CreateModelContext>(scratch);
    context->Platform = platform::GetPlatformContext();
    context->Assets = &context->Platform->Assets;
    context->Options = params;
    context->AssetPath = asset_path;
    context->FullPath = GetFullAssetPath(scratch, assets, asset_path);
    context->Dir = paths::GetDirname(scratch, context->FullPath);
    context->Scene = scene;

    if (!ProcessNode(scoped_arena, context, scene->mRootNode)) {
        SDL_Log("ERROR: Processing model %s\n", full_asset_path.Str());
        return {};
    }

    i32 asset_id = GenerateAssetID(EAssetType::Model, asset_path);
    Model model{
        .ID = asset_id,
        .Path = platform::InternToStringArena(asset_path.Str()),
    };

    ASSERT(model.MeshBindings.Capacity() >= context->MeshBindings.Size);
    for (const ModelMeshBinding& mmb : context->MeshBindings) {
        ASSERT(IsValid(mmb));
        model.MeshBindings.Push(mmb);
    }

    SDL_Log("Created model %s. Meshes: %u\n", full_asset_path.Str(), model.MeshBindings.Size);

    AssetHandle result =
        assets->ModelHolder.PushAsset(asset_id, asset_path, params.AssetOptions, std::move(model));
    return {result};
}

ModelAssetHandle CreateSyntheticModel(AssetRegistry* assets,
                                      String asset_path,
                                      std::span<const ModelMeshBinding> mmb,
                                      const CreateModelParams& params) {
    if (ModelAssetHandle found = FindModelHandle(assets, asset_path); IsValid(found)) {
        return found;
    }

    ASSERT(!assets->ModelHolder.IsFull());

    i32 asset_id = GenerateAssetID(EAssetType::Model, asset_path);
    Model model{
        .ID = asset_id,
        .Path = platform::InternToStringArena(asset_path.Str()),
    };
    model.MeshBindings.Push(mmb);

    SDL_Log("Created model %s. Meshes: %u\n", asset_path.Str(), model.MeshBindings.Size);

    AssetHandle result =
        assets->ModelHolder.PushAsset(asset_id, asset_path, params.AssetOptions, std::move(model));
    return {result};
}

void Draw(AssetRegistry* assets,
          ModelAssetHandle model_handle,
          ShaderAssetHandle shader_handle,
          const RenderState& rs) {
    auto [_, model] = FindModelAsset(assets, model_handle);
    ASSERT(model);

    for (const ModelMeshBinding& mmb : model->MeshBindings) {
        // TODO(cdc): Call an internal draw call that takes the already resolved mesh and shader
        //            handles, rather than being re-evaluating the handles everytime.
        Draw(assets, mmb.MeshHandle, shader_handle, mmb.MaterialHandle, rs);
    }
}

// COMPONENTS --------------------------------------------------------------------------------------

void OnLoadedOnEntity(Entity* entity, StaticModelComponent* smc) {
    ASSERT(entity);
    ASSERT(smc);
    LoadAssets(smc);
}

void Serialize(SerdeArchive* sa, StaticModelComponent* smc) {
    SERDE(sa, smc, ModelPath);
    SERDE(sa, smc, ShaderPath);

    SERDE(sa, smc, ModelHandle);
}

void LoadAssets(StaticModelComponent* smc) {
    auto scratch = GetScratchArena();

    if (!IsValid(smc->ModelHandle)) {
        if (!smc->ModelPath.IsEmpty()) {
            AssetRegistry* assets = &platform::GetPlatformContext()->Assets;
            smc->ModelHandle = CreateModel(assets, smc->ModelPath);
            if (!IsValid(smc->ModelHandle)) {
                SDL_Log("ERROR: Failed to load model %s\n", smc->ModelPath.Str());
            }
        }
    }

    if (!IsValid(smc->ShaderHandle)) {
        if (!smc->ShaderPath.IsEmpty()) {
            AssetRegistry* assets = &platform::GetPlatformContext()->Assets;

            smc->ShaderHandle = CreateShader(assets, smc->ShaderPath);
            if (!IsValid(smc->ShaderHandle)) {
                SDL_Log("ERROR: Failed to load shader %s\n", smc->ShaderPath.Str());
            }
        }
    }
}

void BuildImGui(StaticModelComponent* smc) {
	(void)smc;
    // ImGui::Text("ModelPath: %s", smc->ModelPath.Str());
    // ImGui::Text("ShaderPath: %s", smc->ShaderPath.Str());
}

}  // namespace kdk
