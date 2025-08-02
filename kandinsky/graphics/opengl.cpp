#include <kandinsky/graphics/opengl.h>

#include <kandinsky/defines.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/input.h>
#include <kandinsky/memory.h>
#include <kandinsky/platform.h>
#include <kandinsky/string.h>
#include <kandinsky/time.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_assert.h>

#include <stb/stb_image.h>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include "kandinsky/graphics/model.h"

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

// Base --------------------------------------------------------------------------------------------

namespace opengl_private {

bool LoadInitialShaders(PlatformState* ps) {
    auto scratch = GetScratchArena();

    String source =
        paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/shaders/grid.glsl"));
    if (ps->BaseAssets.GridShader = CreateShader(&ps->Shaders, source);
        !ps->BaseAssets.GridShader) {
        SDL_Log("ERROR: Creating grid shader from %s", source.Str());
        return false;
    }
    glGenVertexArrays(1, &ps->BaseAssets.GridVAO);

    return true;
}

// clang-format off
std::array kCubeVertices = {
    // positions          // normals           // texture coords
	Vertex{{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {0.0f, 0.0f}},
    Vertex{{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {1.0f, 0.0f}},
    Vertex{{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {1.0f, 1.0f}},
    Vertex{{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {1.0f, 1.0f}},
    Vertex{{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {0.0f, 1.0f}},
    Vertex{{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f,},  {0.0f, 0.0f}},

    Vertex{{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {0.0f, 0.0f}},
    Vertex{{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {1.0f, 0.0f}},
    Vertex{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {1.0f, 1.0f}},
    Vertex{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {1.0f, 1.0f}},
    Vertex{{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {0.0f, 1.0f}},
    Vertex{{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f,},  {0.0f, 0.0f}},

    Vertex{{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f,},  {1.0f, 1.0f}},
    Vertex{{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f,},  {0.0f, 0.0f}},
    Vertex{{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f,},  {1.0f, 0.0f}},

    Vertex{{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f,},  {1.0f, 1.0f}},
    Vertex{{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f,},  {0.0f, 0.0f}},
    Vertex{{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f,},  {1.0f, 0.0f}},

    Vertex{{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f,},  {1.0f, 1.0f}},
    Vertex{{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f,},  {0.0f, 0.0f}},
    Vertex{{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f,},  {0.0f, 1.0f}},

    Vertex{{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f,},  {0.0f, 1.0f}},
    Vertex{{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f,},  {1.0f, 1.0f}},
    Vertex{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f,},  {1.0f, 0.0f}},
    Vertex{{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f,},  {0.0f, 0.0f}},
    Vertex{{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f,},  {0.0f, 1.0f}},
};
// clang-format on

bool LoadInitialMeshes(PlatformState* ps) {
    auto scratch = GetScratchArena();

    // Cube.
    {
        CreateMeshOptions options{
            .Vertices = kCubeVertices.data(),
            .VertexCount = (u32)kCubeVertices.size(),
        };
        Mesh* cube_mesh = CreateMesh(&ps->Meshes, "Cube", options);
        if (!cube_mesh) {
            SDL_Log("ERROR: Creating cube mesh");
            return false;
        }

        if (ps->BaseAssets.CubeModel =
                CreateModelFromMesh(&ps->Models, String("/Basic/Cube"), cube_mesh);
            !ps->BaseAssets.CubeModel) {
            SDL_Log("ERROR: Creating cube model from mesh");
            return false;
        }
    }

    // Sphere.
    {
        String path =
            paths::PathJoin(scratch.Arena, ps->BasePath, String("assets/models/sphere/scene.gltf"));
        if (ps->BaseAssets.SphereModel = CreateModel(scratch.Arena, &ps->Models, path);
            !ps->BaseAssets.SphereModel) {
            SDL_Log("ERROR: Creating sphere mesh");
            return false;
        }
    }

    return true;
}

}  // namespace opengl_private

bool LoadBaseAssets(PlatformState* ps) {
    if (!opengl_private::LoadInitialShaders(ps)) {
        SDL_Log("ERROR: Loading initial shaders");
        return false;
    }

    if (!opengl_private::LoadInitialMeshes(ps)) {
        SDL_Log("ERROR: Loading initial meshes");
        return false;
    }

    return true;
}

// Grid --------------------------------------------------------------------------------------------

void DrawGrid(const RenderState& rs, float near, float far) {
    auto* ps = platform::GetPlatformContext();
    if (Shader* grid = ps->BaseAssets.GridShader) {
        Use(*grid);
        glBindVertexArray(ps->BaseAssets.GridVAO);
        SetFloat(*grid, "uGridSize", 75);
        SetVec3(*grid, "uCameraPos", rs.CameraPosition);
        SetVec2(*grid, "uFogRange", {near, far});
        SetMat4(*grid, "uM_View", GetPtr(rs.M_View));
        SetMat4(*grid, "uM_Proj", GetPtr(rs.M_Proj));

        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
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
    ASSERT(registry->LineBatcherCount < LineBatcherRegistry::kMaxLineBatchers);

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

    glBindVertexArray(GL_NONE);

    LineBatcher lb{
        .Name = platform::InternToStringArena(name),
        .ID = IDFromString(name),
        .VAO = vao,
        .VBO = vbo,
    };
    lb.Data.reserve(128);

    registry->LineBatchers[registry->LineBatcherCount++] = std::move(lb);
    return &registry->LineBatchers[registry->LineBatcherCount - 1];
}

LineBatcher* FindLineBatcher(LineBatcherRegistry* registry, u32 id) {
    for (u32 i = 0; i < registry->LineBatcherCount; i++) {
        auto& lb = registry->LineBatchers[i];
        if (lb.ID == id) {
            return &lb;
        }
    }

    return nullptr;
}

// Material ----------------------------------------------------------------------------------------

Material* CreateMaterial(MaterialRegistry* registry, const char* name, const Material& material) {
    ASSERT(registry->MaterialCount < MaterialRegistry::kMaxMaterials);

    u32 id = IDFromString(name);
    if (Material* found = FindMaterial(registry, id)) {
        return found;
    }

    registry->Materials[registry->MaterialCount++] = material;
    Material& result = registry->Materials[registry->MaterialCount - 1];
    result.ID = IDFromString(name);
    return &result;
}

Material* FindMaterial(MaterialRegistry* registry, u32 id) {
    for (u32 i = 0; i < registry->MaterialCount; i++) {
        Material& material = registry->Materials[i];
        if (material.ID == id) {
            return &material;
        }
    }

    return nullptr;
}

// Mesh --------------------------------------------------------------------------------------------

void Draw(const Mesh& mesh,
          const Shader& shader,
          const RenderState& rs,
          const Material* override_material) {
    using namespace opengl_private;

    ASSERT(IsValid(mesh));
    ASSERT(IsValid(shader));

    u32 diffuse_index = 0;
    u32 specular_index = 0;
    u32 emissive_index = 0;

    Use(shader);

    SetUniforms(rs, shader);

    // Setup the textures.
    const Material* material = override_material ? override_material : mesh.Material;
    if (material) {
        SetVec3(shader, "uMaterial.Albedo", material->Albedo);
        SetVec3(shader, "uMaterial.Diffuse", material->Diffuse);
        SetFloat(shader, "uMaterial.Shininess", material->Shininess);

        for (u32 texture_index = 0; texture_index < material->TextureCount; texture_index++) {
            if (!material->Textures[texture_index]) {
                glActiveTexture(GL_TEXTURE0 + texture_index);
                glBindTexture(GL_TEXTURE_2D, NULL);
                continue;
            }

            const Texture& texture = *material->Textures[texture_index];
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
    } else {
        SetVec3(shader, "uMaterial.Albedo", Vec3(0.1f));
        SetVec3(shader, "uMaterial.Diffuse", Vec3(0.1f));
        SetFloat(shader, "uMaterial.Shininess", 0);
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
    ASSERT(registry->MeshCount < MeshRegistry::kMaxMeshes);
    u32 id = IDFromString(name);
    if (Mesh* found = FindMesh(registry, name)) {
        return found;
    }

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
        .ID = id,
        .VAO = vao,
        .VertexCount = options.VertexCount,
        .IndexCount = options.IndexCount,
        .Material = options.Material,
    };

    SDL_Log("Created mesh %s. Vertices %u, Indices: %u\n", name, mesh.VertexCount, mesh.IndexCount);

    registry->Meshes[registry->MeshCount++] = std::move(mesh);
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

void SetUVec2(const Shader& shader, const char* uniform, const UVec2& value) {
    ASSERT(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform2ui(location, value[0], value[1]);
}

void SetUVec3(const Shader& shader, const char* uniform, const UVec3& value) {
    ASSERT(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform3ui(location, value[0], value[1], value[2]);
}

void SetUVec4(const Shader& shader, const char* uniform, const UVec4& value) {
    ASSERT(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform4ui(location, value[0], value[1], value[2], value[3]);
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

GLuint CompileShader(String path, GLuint shader_type, String source) {
    auto scratch = GetScratchArena();

    const char* shader_type_str = nullptr;
    String processed_source = {};
    if (shader_type == GL_VERTEX_SHADER) {
        shader_type_str = "VERTEX";

        String header(R"(
#version 430 core
#define VERTEX_SHADER
)");
        processed_source = Concat(scratch.Arena, header, source);
    } else if (shader_type == GL_FRAGMENT_SHADER) {
        shader_type_str = "FRAGMENT";

        String header(R"(
#version 430 core
#define FRAGMENT_SHADER
)");
        processed_source = Concat(scratch.Arena, header, source);
    } else {
        SDL_Log("ERROR: Unsupported shader type %d\n", shader_type);
        return GL_NONE;
    }

    const char* src = processed_source.Str();
    unsigned int handle = glCreateShader(shader_type);
    glShaderSource(handle, 1, &src, NULL);
    glCompileShader(handle);

    int success = 0;
    char log[512];

    glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(handle, sizeof(log), NULL, log);
        SDL_Log("ERROR: Compiling shader program %s: compiling %s shader: %s\n",
                path.Str(),
                shader_type_str,
                log);
        SDL_Log("SHADER -------------------\n%s", src);
        return GL_NONE;
    }

    return handle;
}

Shader CreateNewShader(u32 id, String path, String source) {
    GLuint vs = CompileShader(path, GL_VERTEX_SHADER, source);
    if (vs == GL_NONE) {
        SDL_Log("ERROR: Compiling vertex shader");
        return {};
    }
    DEFER { glDeleteShader(vs); };

    GLuint fs = CompileShader(path, GL_FRAGMENT_SHADER, source);
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
        .ID = id,
        .Path = platform::InternToStringArena(path.Str()),
        .Program = program,
    };

    bool ok = SDL_GetCurrentTime(&shader.LastLoadTime);
    ASSERT(ok);

    return shader;
}

}  // namespace opengl_private

Shader* CreateShader(ShaderRegistry* registry, String path) {
    using namespace opengl_private;

    u32 id = IDFromString(path.Str());
    if (Shader* found = FindShader(registry, id)) {
        return found;
    }

    ASSERT(registry->ShaderCount < ShaderRegistry::kMaxShaders);

    void* source = SDL_LoadFile(path.Str(), nullptr);
    if (!source) {
        SDL_Log("ERROR: reading shader at %s: %s\n", path.Str(), SDL_GetError());
        return nullptr;
    }
    DEFER { SDL_free(source); };

    // Create the shader.
    Shader shader = CreateNewShader(id, path, String((const char*)source));
    if (!IsValid(shader)) {
        return nullptr;
    }

    SDL_Log("Created shader %s\n", path.Str());
    registry->Shaders[registry->ShaderCount++] = std::move(shader);
    return &registry->Shaders[registry->ShaderCount - 1];
}

Shader* FindShader(ShaderRegistry* registry, u32 id) {
    for (u32 i = 0; i < registry->ShaderCount; i++) {
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
    bool should_reload = false;

    const char* path = shader->Path.Str();
    SDL_Log("Re-evaluating shader %s", shader->Path.Str());
    if (IsShaderPathMoreRecent(*shader, path)) {
        should_reload = true;
    }

    if (!should_reload) {
        SDL_Log("Shader %s up to date", shader->Path.Str());
        return true;
    }
    SDL_Log("Shader %s is not up to date. Reloading", shader->Path.Str());

    void* source = SDL_LoadFile(path, nullptr);
    if (!source) {
        SDL_Log("ERROR: reading shader at %s: %s\n", path, SDL_GetError());
        return false;
    }
    DEFER { SDL_free(source); };

    // We create a new shader with the new source.
    u32 id = IDFromString(path);
    Shader new_shader = CreateNewShader(id, shader->Path, String((const char*)source));
    if (!IsValid(new_shader)) {
        SDL_Log("ERROR: Creating new shader for %s", shader->Path.Str());
        return false;
    }

    // Now that we have a valid shader, we can delete the current one and swap the value.
    glDeleteProgram(shader->Program);
    shader->Program = new_shader.Program;
    shader->LastLoadTime = new_shader.LastLoadTime;

    SDL_Log("Reloaded shader %s", shader->Path.Str());

    return true;
}

}  // namespace opengl_private

bool ReevaluateShaders(ShaderRegistry* registry) {
    using namespace opengl_private;

    for (u32 i = 0; i < registry->ShaderCount; i++) {
        Shader& shader = registry->Shaders[i];
        if (!ReevaluateShader(&shader)) {
            SDL_Log("ERROR: Re-evaluating shader %d: %s", i, shader.Path.Str());
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
    ASSERT(registry->TextureCount < TextureRegistry::kMaxTextures);
    u32 id = IDFromString(path);
    if (Texture* found = FindTexture(registry, id)) {
        return found;
    }

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
        .Path = platform::InternToStringArena(path),
        .ID = id,
        .Width = width,
        .Height = height,
        .Handle = handle,
        .Type = options.Type,
    };
    registry->Textures[registry->TextureCount++] = std::move(texture);

    SDL_Log("Created texture %s: %s\n", name, path);

    return &registry->Textures[registry->TextureCount - 1];
}

Texture* FindTexture(TextureRegistry* registry, u32 id) {
    for (u32 i = 0; i < registry->TextureCount; i++) {
        auto& texture = registry->Textures[i];
        if (texture.ID == id) {
            return &texture;
        }
    }

    return nullptr;
}
}  // namespace kdk
