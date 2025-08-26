#include <kandinsky/graphics/opengl.h>

#include <kandinsky/core/defines.h>
#include <kandinsky/core/file.h>
#include <kandinsky/core/memory.h>
#include <kandinsky/core/string.h>
#include <kandinsky/core/time.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/input.h>
#include <kandinsky/platform.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_assert.h>

#include <stb/stb_image.h>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include "kandinsky/graphics/model.h"

namespace kdk {

// Grid --------------------------------------------------------------------------------------------

void DrawGrid(const RenderState& rs, float near, float far) {
    auto* ps = platform::GetPlatformContext();
    if (Shader* grid = ps->Assets.BaseAssets.GridShader) {
        Use(*grid);
        glBindVertexArray(ps->Assets.BaseAssets.GridVAO);
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

LineBatcher* FindLineBatcher(LineBatcherRegistry* registry, i32 id) {
    for (i32 i = 0; i < registry->LineBatcherCount; i++) {
        auto& lb = registry->LineBatchers[i];
        if (lb.ID == id) {
            return &lb;
        }
    }

    return nullptr;
}

// Material ----------------------------------------------------------------------------------------

Material* CreateMaterial(MaterialRegistry* registry, String name, const Material& material) {
    ASSERT(registry->MaterialCount < MaterialRegistry::kMaxMaterials);

    i32 id = IDFromString(name);
    ASSERT(id != NONE);
    if (Material* found = FindMaterial(registry, id)) {
        return found;
    }

    registry->Materials[registry->MaterialCount++] = material;
    Material& result = registry->Materials[registry->MaterialCount - 1];
    result.ID = id;
    return &result;
}

Material* FindMaterial(MaterialRegistry* registry, i32 id) {
    for (i32 i = 0; i < registry->MaterialCount; i++) {
        Material& material = registry->Materials[i];
        if (material.ID == id) {
            return &material;
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

Shader CreateNewShader(i32 id, String path, String source) {
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

    i32 id = IDFromString(path);
    if (Shader* found = FindShader(registry, id)) {
        return found;
    }

    ASSERT(registry->ShaderCount < ShaderRegistry::kMaxShaders);

    auto scratch = GetScratchArena();

    auto source = LoadFile(scratch.Arena, path);
    if (source.empty()) {
        SDL_Log("ERROR: reading shader at %s: %s\n", path.Str(), SDL_GetError());
        return nullptr;
    }

    // Create the shader.
    Shader shader = CreateNewShader(id, path, String(source));
    if (!IsValid(shader)) {
        return nullptr;
    }

    SDL_Log("Created shader %s\n", path.Str());
    registry->Shaders[registry->ShaderCount++] = std::move(shader);
    return &registry->Shaders[registry->ShaderCount - 1];
}

Shader* FindShader(ShaderRegistry* registry, i32 id) {
    for (i32 i = 0; i < registry->ShaderCount; i++) {
        auto& shader = registry->Shaders[i];
        if (shader.ID == id) {
            return &shader;
        }
    }

    return nullptr;
}

namespace opengl_private {

bool IsShaderPathMoreRecent(const Shader& shader, String path) {
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path.Str(), &info)) {
        SDL_Log("ERROR: Getting path info for %s: %s", path.Str(), SDL_GetError());
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

    SDL_Log("Re-evaluating shader %s", shader->Path.Str());
    if (IsShaderPathMoreRecent(*shader, shader->Path)) {
        should_reload = true;
    }

    if (!should_reload) {
        SDL_Log("Shader %s up to date", shader->Path.Str());
        return true;
    }
    SDL_Log("Shader %s is not up to date. Reloading", shader->Path.Str());

    auto scratch = GetScratchArena();

    auto source = LoadFile(scratch.Arena, shader->Path);
    if (source.empty()) {
        SDL_Log("ERROR: reading shader at %s: %s\n", shader->Path.Str(), SDL_GetError());
        return false;
    }

    // We create a new shader with the new source.
    i32 id = IDFromString(shader->Path.Str());
    Shader new_shader = CreateNewShader(id, shader->Path, String(source));
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

    for (i32 i = 0; i < registry->ShaderCount; i++) {
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
    i32 id = IDFromString(path);
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

Texture* FindTexture(TextureRegistry* registry, i32 id) {
    for (i32 i = 0; i < registry->TextureCount; i++) {
        auto& texture = registry->Textures[i];
        if (texture.ID == id) {
            return &texture;
        }
    }

    return nullptr;
}
}  // namespace kdk
