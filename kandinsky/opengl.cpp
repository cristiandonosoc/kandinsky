#include <kandinsky/opengl.h>

#include <kandinsky/defines.h>
#include <kandinsky/input.h>
#include <kandinsky/platform.h>
#include <kandinsky/print.h>
#include <kandinsky/string.h>
#include <kandinsky/time.h>
#include <kandinsky/utils/defer.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_assert.h>

#include <stb/stb_image.h>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/importer.hpp>

#include <array>

namespace kdk {

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

// Mouse -------------------------------------------------------------------------------------------

void Update(PlatformState* ps, Camera* camera, double dt) {
    constexpr float kMaxPitch = ToRadians(89.0f);

    if (MOUSE_DOWN(ps, MIDDLE)) {
        Vec2 offset = ps->InputState.MouseMove * camera->MouseSensitivity;

        camera->Yaw += ToRadians(offset.x);
        camera->Yaw = FMod(camera->Yaw, ToRadians(360.0f));

        camera->Pitch -= ToRadians(offset.y);
        camera->Pitch = Clamp(camera->Pitch, -kMaxPitch, kMaxPitch);
    }

    Vec3 dir;
    dir.x = cos(camera->Yaw) * cos(camera->Pitch);
    dir.y = sin(camera->Pitch);
    dir.z = sin(camera->Yaw) * cos(camera->Pitch);
    camera->Front = Normalize(dir);

    camera->Right = Normalize(Cross(camera->Front, Vec3(0.0f, 1.0f, 0.0f)));
    camera->Up = Normalize(Cross(camera->Right, camera->Front));

    float speed = camera->MovementSpeed * (float)dt;
    if (KEY_DOWN(ps, W)) {
        camera->Position += speed * camera->Front;
    }
    if (KEY_DOWN(ps, S)) {
        camera->Position -= speed * camera->Front;
    }
    if (KEY_DOWN(ps, A)) {
        camera->Position -= speed * camera->Right;
    }
    if (KEY_DOWN(ps, D)) {
        camera->Position += speed * camera->Right;
    }

    camera->View = LookAt(camera->Position, camera->Position + camera->Front, camera->Up);

    float aspect_ratio = (float)(ps->Window.Width) / (float)(ps->Window.Height);
    camera->Proj = Perspective(ToRadians(45.0f), aspect_ratio, 0.1f, 100.0f);

    camera->ViewProj = camera->Proj * camera->View;
}

// LineBatcher -------------------------------------------------------------------------------------

void Reset(LineBatcher* lb) {
    lb->Batches.clear();
    lb->Data.clear();
}

void StartLineBatch(LineBatcher* lb, GLenum mode, Color32 color, float line_width) {
    ASSERT(IsValid(*lb));
    ASSERT(lb->CurrentBatch == NONE);
    lb->Batches.push_back({
        .Mode = mode,
        .Color = ToVec4(color),
        .LineWidth = line_width,
    });
    lb->CurrentBatch = static_cast<i32>(lb->Batches.size() - 1);
}

void EndLineBatch(LineBatcher* lb) {
    ASSERT(IsValid(*lb));
    ASSERT(lb->CurrentBatch != NONE);
    lb->CurrentBatch = NONE;
}

void AddPoint(LineBatcher* lb, const Vec3& point) {
    ASSERT(IsValid(*lb));
    ASSERT(lb->CurrentBatch != NONE);

    lb->Batches[lb->CurrentBatch].PrimitiveCount++;

    auto* begin = &point;
    auto* end = begin + 1;
    lb->Data.insert(lb->Data.end(), (u8*)begin, (u8*)end);
}

void AddPoints(LineBatcher* lb, const Vec3& p1, const Vec3& p2) {
    AddPoint(lb, p1);
    AddPoint(lb, p2);
}

void AddPoints(LineBatcher* lb, std::span<const Vec3> points) {
    ASSERT(IsValid(*lb));

    for (const auto& point : points) {
        AddPoint(lb, point);
    }
}

void Buffer(PlatformState*, const LineBatcher& lb) {
    ASSERT(IsValid(lb));

    // Send the data.
    glBindBuffer(GL_ARRAY_BUFFER, lb.VBO);
    glBufferData(GL_ARRAY_BUFFER, lb.Data.size(), lb.Data.data(), GL_STREAM_DRAW);
}

void Draw(const LineBatcher& lb, const Shader& shader) {
    ASSERT(IsValid(lb));

    // Ensure we leave line width as it was.
    float current_line_width = 1.0f;
    glGetFloatv(GL_LINE_WIDTH, &current_line_width);

    glBindVertexArray(lb.VAO);

    GLint primitive_count = 0;
    for (const LineBatch& batch : lb.Batches) {
        SetVec4(shader, "uColor", batch.Color);
        glLineWidth(batch.LineWidth);

        glDrawArrays(batch.Mode, primitive_count, batch.PrimitiveCount);
        primitive_count += batch.PrimitiveCount;
    }

    glLineWidth(current_line_width);
}

LineBatcher* CreateLineBatcher(LineBatcherRegistry* registry, const char* name) {
    GLuint vao = GL_NONE;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo = GL_NONE;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    GLsizei stride = 3 * sizeof(float);
    u64 offset = 0;
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
    glEnableVertexAttribArray(0);

    /* offset += 3 * sizeof(float); */
    /* glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset); */
    /* glEnableVertexAttribArray(1); */

    glBindVertexArray(GL_NONE);

    // We "intern" the string.
    LineBatcher lb{
        .Name = platform::InternToStringArena(name),
        .ID = IDFromString(name),
        .VAO = vao,
        .VBO = vbo,
    };
    lb.Data.reserve(128);

    registry->LineBatchers[registry->Count] = std::move(lb);
    registry->Count++;

    return &registry->LineBatchers[registry->Count - 1];
}

LineBatcher* FindLineBatcher(LineBatcherRegistry* registry, u32 id) {
    for (u32 i = 0; i < registry->Count; i++) {
        auto& lb = registry->LineBatchers[i];
        if (lb.ID == id) {
            return &lb;
        }
    }

    return nullptr;
}

// Mesh --------------------------------------------------------------------------------------------

void Draw(const Mesh& mesh, const Shader& shader) {
    using namespace opengl_private;

    ASSERT(IsValid(mesh));
    ASSERT(IsValid(shader));

    u32 diffuse_index = 0;
    u32 specular_index = 0;
    u32 emissive_index = 0;

    Use(shader);

    // Setup the textures.
    for (u32 texture_index = 0; texture_index < std::size(mesh.Textures); texture_index++) {
        if (!mesh.Textures[texture_index]) {
            glActiveTexture(GL_TEXTURE0 + texture_index);
            glBindTexture(GL_TEXTURE_2D, NULL);
            continue;
        }

        const Texture& texture = *mesh.Textures[texture_index];
        ASSERT(IsValid(texture));

        glActiveTexture(GL_TEXTURE0 + texture_index);
        glBindTexture(GL_TEXTURE_2D, texture.Handle);

        switch (texture.Type) {
            case ETextureType::None: continue;
            case ETextureType::Diffuse: {
                ASSERT(diffuse_index < kDiffuseSamplerNames.size());
                SetI32(shader, kDiffuseSamplerNames[diffuse_index], texture_index);
                diffuse_index++;
                break;
            }
            case ETextureType::Specular: {
                ASSERT(specular_index < kSpecularSamplerNames.size());
                SetI32(shader, kSpecularSamplerNames[specular_index], texture_index);
                specular_index++;
                break;
            }
            case ETextureType::Emissive: {
                ASSERT(emissive_index < kEmissiveSamplerNames.size());
                SetI32(shader, kEmissiveSamplerNames[emissive_index], texture_index);
                emissive_index++;
                break;
            }
        }
    }

    // Make the draw call.
    glBindVertexArray(mesh.VAO);
    if (mesh.IndexCount == 0) {
        glDrawArrays(GL_TRIANGLES, 0, mesh.VertexCount);
    } else {
        glDrawElements(GL_TRIANGLES, mesh.IndexCount, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(NULL);
}

Mesh* CreateMesh(MeshRegistry* registry, const char* name, const CreateMeshOptions& options) {
    if (options.VertexCount == 0) {
        return nullptr;
    }

    GLuint vao = GL_NONE;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Copy our vertices into a Vertex Buffer Object (VBO).
    GLuint vbo = GL_NONE;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 options.VertexCount * sizeof(Vertex),
                 options.Vertices,
                 options.MemoryUsage);

    if (options.IndexCount > 0) {
        GLuint ebo = GL_NONE;
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     options.IndexCount * sizeof(u32),
                     options.Indices,
                     options.MemoryUsage);
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
        .Name = platform::InternToStringArena(name),
        .ID = IDFromString(name),
        .VAO = vao,
        .VertexCount = options.VertexCount,
        .IndexCount = options.IndexCount,
    };
    std::memcpy(mesh.Textures, options.Textures, sizeof(options.Textures));

    registry->Meshes[registry->MeshCount] = std::move(mesh);
    registry->MeshCount++;

    return &registry->Meshes[registry->MeshCount - 1];
}

Mesh* FindMesh(MeshRegistry* registry, u32 id) {
    for (u32 i = 0; i < registry->MeshCount; i++) {
        auto& mesh = registry->Meshes[i];
        if (mesh.ID == id) {
            return &mesh;
        }
    }

    return nullptr;
}

// Model -------------------------------------------------------------------------------------------

namespace opengl_private {

struct CreateModelContext {
	PlatformState* ps = nullptr;

	String Name = {};
	String Path = {};
	String Dir = {};

	const aiScene* Scene = nullptr;

    std::array<Mesh*, 1024> Meshes = {};
    u32 MeshCount = 0;
};

Mesh* ProcessMesh(Arena* arena, CreateModelContext* context, aiMesh* aimesh) {
	auto scratch = GetScratchArena(arena);
	DEFER { ReleaseScratchArena(&scratch); };

	const char* mesh_name = Printf(scratch.Arena, "%s_%d", context->Name.Str, context->MeshCount);
	if (Mesh* found = FindMesh(&context->ps->Meshes, mesh_name)) {
		return found;
	}

    // Process the vertices.
    auto* vertices = (Vertex*)ArenaPushArray<Vertex>(arena, aimesh->mNumVertices);
    Vertex* vertex_ptr = vertices;
    u32 vertex_count = aimesh->mNumVertices;
    for (u32 i = 0; i < aimesh->mNumVertices; i++) {
        *vertex_ptr = {};
        std::memcpy(&vertex_ptr->Position, &aimesh->mVertices[i], sizeof(Vec3));
        std::memcpy(&vertex_ptr->Normal, &aimesh->mNormals[i], sizeof(Vec3));
        if (aimesh->mTextureCoords[0]) {
            std::memcpy(&vertex_ptr->UVs, &aimesh->mTextureCoords[0][i], sizeof(Vec2));
        }

        vertex_ptr++;
    }
    ASSERT(vertex_ptr == (vertices + aimesh->mNumVertices));

    SDL_Log("Mesh %s: Loaded %d vertices\n", mesh_name, vertex_count);

    // Process the indices.
    // We make a first pass to know how much to allocate.
    u32 index_count = 0;
    for (u32 i = 0; i < aimesh->mNumFaces; i++) {
        index_count += aimesh->mFaces[i].mNumIndices;
    }

    // Now we can collect the indices in one nice array.
    // TODO(cdc): Likely there is a clever way to join the arena allocations.
    u32* indices = (u32*)ArenaPushArray<u32>(arena, index_count);
    u32* index_ptr = indices;
    for (u32 i = 0; i < aimesh->mNumFaces; i++) {
        const aiFace& face = aimesh->mFaces[i];
        std::memcpy(index_ptr, face.mIndices, face.mNumIndices * sizeof(u32));
        index_ptr += face.mNumIndices;
    }
    ASSERT(index_ptr == (indices + index_count));

    SDL_Log("Mesh %s: Loaded %d indices\n", mesh_name, index_count);

    // Load the textures.
    aiMaterial* material = context->Scene->mMaterials[aimesh->mMaterialIndex];

    for (u32 i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++) {
        aiString path;
        material->GetTexture(aiTextureType_DIFFUSE, i, &path);
        SDL_Log("Mesh %s: Texture %s\n", mesh_name, path.C_Str());
    }

    return nullptr;
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

Model* CreateModel(Arena* arena, ModelRegistry*, const char* name, const char* path) {
    using namespace opengl_private;

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        SDL_Log("ERROR: CreateModel: %s\n", importer.GetErrorString());
        return nullptr;
    }

    SDL_Log("Model %s\n", path);

    auto scratch = GetScratchArena(arena);
    DEFER { ReleaseScratchArena(&scratch); };

    auto* context = ArenaPush<CreateModelContext>(scratch.Arena);
	context->Name = String(name);
    context->Path = String(path);
    context->Dir = paths::GetDirname(scratch.Arena, context->Path);
	context->Scene = scene;

    ProcessNode(arena, context, scene->mRootNode);

    SDL_Log("Used %llu bytes\n", arena->Offset);

    return nullptr;
}

// Shader ------------------------------------------------------------------------------------------

void Use(const Shader& shader) {
    ASSERT(IsValid(shader));
    glUseProgram(shader.Program);
}

void SetBool(const Shader& shader, const char* uniform, bool value) {
    ASSERT(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform1i(location, static_cast<i32>(value));
}

void SetI32(const Shader& shader, const char* uniform, i32 value) {
    ASSERT(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform1i(location, value);
}

void SetU32(const Shader& shader, const char* uniform, u32 value) {
    ASSERT(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform1ui(location, value);
}

void SetFloat(const Shader& shader, const char* uniform, float value) {
    ASSERT(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform1f(location, value);
}

void SetVec2(const Shader& shader, const char* uniform, const Vec2& value) {
    ASSERT(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform2f(location, value[0], value[1]);
}

void SetVec3(const Shader& shader, const char* uniform, const Vec3& value) {
    ASSERT(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform3f(location, value[0], value[1], value[2]);
}

void SetVec4(const Shader& shader, const char* uniform, const Vec4& value) {
    ASSERT(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform4f(location, value[0], value[1], value[2], value[3]);
}

void SetMat4(const Shader& shader, const char* uniform, const float* value) {
    ASSERT(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniformMatrix4fv(location, 1, GL_FALSE, value);
}

namespace opengl_private {

GLuint CompileShader(const char* name, const char* type, GLuint shader_type, const char* source) {
    unsigned int handle = glCreateShader(shader_type);
    glShaderSource(handle, 1, &source, NULL);
    glCompileShader(handle);

    int success = 0;
    char log[512];

    glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(handle, sizeof(log), NULL, log);
        SDL_Log("ERROR: Compiling shader program %s: compiling %s shader: %s\n", name, type, log);
        return GL_NONE;
    }

    return handle;
}

Shader CreateNewShader(const char* name, const char* vert_source, const char* frag_source) {
    GLuint vs = CompileShader(name, "vertex", GL_VERTEX_SHADER, vert_source);
    if (vs == GL_NONE) {
        SDL_Log("ERROR: Compiling vertex shader");
        return {};
    }
    DEFER { glDeleteShader(vs); };

    GLuint fs = CompileShader(name, "fragment", GL_FRAGMENT_SHADER, frag_source);
    if (fs == GL_NONE) {
        SDL_Log("ERROR: Compiling fragment shader");
        return {};
    }
    DEFER { glDeleteShader(fs); };

    int success = 0;
    char log[512];

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        SDL_Log("ERROR: Linking program: %s\n", log);
        return {};
    }

    Shader shader{
        .Name = platform::InternToStringArena(name),
        .ID = IDFromString(name),
        .Program = program,
    };

    bool ok = SDL_GetCurrentTime(&shader.LastLoadTime);
    ASSERT(ok);

    return shader;
}

}  // namespace opengl_private

Shader* CreateShader(ShaderRegistry* registry,
                     const char* name,
                     const char* vert_path,
                     const char* frag_path) {
    void* vert_source = SDL_LoadFile(vert_path, nullptr);
    if (!vert_source) {
        SDL_Log("ERROR: reading vertex shader at %s: %s\n", vert_path, SDL_GetError());
        return nullptr;
    }
    DEFER { SDL_free(vert_source); };

    void* frag_source = SDL_LoadFile(frag_path, nullptr);
    if (!frag_source) {
        SDL_Log("ERROR: reading fragment shader at %s: %s\n", frag_path, SDL_GetError());
        return nullptr;
    }
    DEFER { SDL_free(frag_source); };

    Shader* shader = CreateShaderFromString(registry,
                                            name,
                                            static_cast<const char*>(vert_source),
                                            static_cast<const char*>(frag_source));
    shader->VertPath = vert_path;
    shader->FragPath = frag_path;

    return shader;
}

Shader* CreateShaderFromString(ShaderRegistry* registry,
                               const char* name,
                               const char* vert_source,
                               const char* frag_source) {
    using namespace opengl_private;

    Shader shader = CreateNewShader(name, vert_source, frag_source);
    registry->Shaders[registry->Count] = std::move(shader);
    registry->Count++;

    return &registry->Shaders[registry->Count - 1];
}

Shader* FindShader(ShaderRegistry* registry, u32 id) {
    for (u32 i = 0; i < registry->Count; i++) {
        auto& shader = registry->Shaders[i];
        if (shader.ID == id) {
            return &shader;
        }
    }

    return nullptr;
}

namespace opengl_private {

bool IsShaderPathMoreRecent(const Shader& shader, const char* path) {
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path, &info)) {
        SDL_Log("ERROR: Getting path info for %s: %s", path, SDL_GetError());
        return false;
    }

#if UNCOMMENT_FOR_DEBUGGING
    std::string shader_load_time = PrintAsDate(shader.LastLoadTime);
    std::string file_load_time = PrintAsDate(info.modify_time);

    SDL_Log("Shader %s. LoadTime: %s, File: %s (%s)",
            shader.Name,
            shader_load_time.c_str(),
            file_load_time.c_str(),
            path);
#endif

    if (info.modify_time > shader.LastLoadTime) {
        return true;
    }

    return false;
}

// Will change the shader contents if succcesful, deleting the old program and loading a new one.
// Will leave the shader intact otherwise.
bool ReevaluateShader(Shader* shader) {
    SDL_Log("Re-evaluating shader %s", shader->Name);

    bool should_reload = false;
    const char* vert_path = shader->VertPath.c_str();
    if (IsShaderPathMoreRecent(*shader, vert_path)) {
        should_reload = true;
    }

    const char* frag_path = shader->FragPath.c_str();
    if (!should_reload && IsShaderPathMoreRecent(*shader, frag_path)) {
        should_reload = true;
    }

    if (!should_reload) {
        SDL_Log("Shader %s up to date", shader->Name);
        return true;
    }
    SDL_Log("Shader %s is not up to date. Reloading", shader->Name);

    void* vert_source = SDL_LoadFile(vert_path, nullptr);
    if (!vert_source) {
        SDL_Log("ERROR: reading vertex shader at %s: %s\n", vert_path, SDL_GetError());
        return false;
    }
    DEFER { SDL_free(vert_source); };

    void* frag_source = SDL_LoadFile(shader->FragPath.c_str(), nullptr);
    if (!frag_source) {
        SDL_Log("ERROR: reading fragment shader at %s: %s\n", frag_path, SDL_GetError());
        return false;
    }
    DEFER { SDL_free(frag_source); };

    // We create a new shader with the new source.

    Shader new_shader =
        CreateNewShader(shader->Name, (const char*)vert_source, (const char*)frag_source);
    if (!IsValid(new_shader)) {
        SDL_Log("ERROR: Creating new shader for %s", shader->Name);
        return false;
    }

    // Now that we have a valid shader, we can delete the current one and swap the value.
    glDeleteProgram(shader->Program);
    shader->Program = new_shader.Program;
    shader->LastLoadTime = new_shader.LastLoadTime;

    SDL_Log("Reloaded shader %s", shader->Name);

    return true;
}

}  // namespace opengl_private

bool ReevaluateShaders(ShaderRegistry* registry) {
    using namespace opengl_private;

    for (u32 i = 0; i < registry->Count; i++) {
        Shader& shader = registry->Shaders[i];
        if (!ReevaluateShader(&shader)) {
            SDL_Log("ERROR: Re-evaluating shader %d: %s", i, shader.Name);
            return true;
        }
    }

    return true;
}

// Texture -----------------------------------------------------------------------------------------

bool IsValid(const Texture& texture) {
    return texture.Width != 0 && texture.Height != 0 && texture.Handle != GL_NONE;
}

void Bind(const Texture& texture, GLuint texture_unit) {
    ASSERT(IsValid(texture));
    glActiveTexture(texture_unit);
    glBindTexture(GL_TEXTURE_2D, texture.Handle);
}

Texture* CreateTexture(TextureRegistry* registry,
                       const char* name,
                       const char* path,
                       const LoadTextureOptions& options) {
    stbi_set_flip_vertically_on_load(options.FlipVertically);

    i32 width, height, channels;
    u8* data = stbi_load(path, &width, &height, &channels, 0);
    if (!data) {
        return nullptr;
    }
    DEFER { stbi_image_free(data); };

    GLuint handle = GL_NONE;
    glGenTextures(1, &handle);
    if (handle == GL_NONE) {
        return nullptr;
    }

    glBindTexture(GL_TEXTURE_2D, handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, options.WrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, options.WrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLuint format = GL_NONE;
    switch (channels) {
        case 3: format = GL_RGB; break;
        case 4: format = GL_RGBA; break;
        default:
            SDL_Log("ERROR: Unsupported number of channels: %d", channels);
            return nullptr;
            break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    Texture texture{
        .Name = platform::InternToStringArena(name),
        .ID = IDFromString(name),
        .Width = width,
        .Height = height,
        .Handle = handle,
        .Type = options.Type,
    };

    registry->Textures[registry->Count] = std::move(texture);
    registry->Count++;

    return &registry->Textures[registry->Count - 1];
}

Texture* FindTexture(TextureRegistry* registry, u32 id) {
    for (u32 i = 0; i < registry->Count; i++) {
        auto& texture = registry->Textures[i];
        if (texture.ID == id) {
            return &texture;
        }
    }

    return nullptr;
}
}  // namespace kdk
