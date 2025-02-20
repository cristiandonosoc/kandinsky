#include <kandinsky/opengl.h>

#include <kandinsky/input.h>
#include <kandinsky/platform.h>
#include <kandinsky/time.h>
#include <kandinsky/utils/defer.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_assert.h>

#include <stb/stb_image.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cassert>
#include <format>

namespace kdk {

std::string ToString(const glm::vec3& vec) {
    return std::format("({}, {}, {})", vec.x, vec.y, vec.z);
}

// Mouse -------------------------------------------------------------------------------------------

void Update(PlatformState* ps, Camera* camera, double dt) {
    constexpr float kMaxPitch = glm::radians(89.0f);

    if (MOUSE_PRESSED(ps, MIDDLE)) {
        glm::vec2 offset = ps->InputState.MouseMove * camera->MouseSensitivity;

        camera->Yaw += glm::radians(offset.x);
        camera->Yaw = glm::mod(camera->Yaw, glm::radians(360.0f));

        camera->Pitch -= glm::radians(offset.y);
        camera->Pitch = glm::clamp(camera->Pitch, -kMaxPitch, kMaxPitch);
    }

    glm::vec3 dir;
    dir.x = cos(camera->Yaw) * cos(camera->Pitch);
    dir.y = sin(camera->Pitch);
    dir.z = sin(camera->Yaw) * cos(camera->Pitch);
    camera->Front = glm::normalize(dir);

    camera->Right = glm::normalize(glm::cross(camera->Front, glm::vec3(0.0f, 1.0f, 0.0f)));
    camera->Up = glm::normalize(glm::cross(camera->Right, camera->Front));

    float speed = camera->MovementSpeed * (float)dt;
    if (KEY_PRESSED(ps, W)) {
        camera->Position += speed * camera->Front;
    }
    if (KEY_PRESSED(ps, S)) {
        camera->Position -= speed * camera->Front;
    }
    if (KEY_PRESSED(ps, A)) {
        camera->Position -= speed * camera->Right;
    }
    if (KEY_PRESSED(ps, D)) {
        camera->Position += speed * camera->Right;
    }
}

glm::mat4 GetViewMatrix(const Camera& camera) {
    return glm::lookAt(camera.Position, camera.Position + camera.Front, camera.Up);
}

// LineBatcher -------------------------------------------------------------------------------------

void Reset(LineBatcher* lb) {
    lb->Batches.clear();
    lb->Data.clear();
}

void StartLineBatch(LineBatcher* lb, GLenum mode, Color32 color, float line_width) {
    assert(IsValid(*lb));
    assert(lb->CurrentBatch == NONE);
    lb->Batches.push_back({
        .Mode = mode,
        .Color = ToVec4(color),
        .LineWidth = line_width,
    });
    lb->CurrentBatch = static_cast<i32>(lb->Batches.size() - 1);
}

void EndLineBatch(LineBatcher* lb) {
    assert(IsValid(*lb));
    assert(lb->CurrentBatch != NONE);
    lb->CurrentBatch = NONE;
}

void AddPoint(LineBatcher* lb, const glm::vec3& point) {
    assert(IsValid(*lb));
    assert(lb->CurrentBatch != NONE);

    lb->Batches[lb->CurrentBatch].PrimitiveCount++;

    auto* begin = &point;
    auto* end = begin + 1;
    lb->Data.insert(lb->Data.end(), (u8*)begin, (u8*)end);
}

void AddPoints(LineBatcher* lb, const glm::vec3& p1, const glm::vec3& p2) {
    AddPoint(lb, p1);
    AddPoint(lb, p2);
}

void AddPoints(LineBatcher* lb, std::span<const glm::vec3> points) {
    assert(IsValid(*lb));

    for (const auto& point : points) {
        AddPoint(lb, point);
    }
}

void Buffer(PlatformState*, const LineBatcher& lb) {
    assert(IsValid(lb));

    // Send the data.
    glBindBuffer(GL_ARRAY_BUFFER, lb.VBO);
    glBufferData(GL_ARRAY_BUFFER, lb.Data.size(), lb.Data.data(), GL_STREAM_DRAW);
}

void Draw(PlatformState* ps, const Shader& shader, const LineBatcher& lb) {
    assert(IsValid(lb));

    // Ensure we leave line width as it was.
    float current_line_width = 1.0f;
    glGetFloatv(GL_LINE_WIDTH, &current_line_width);

    glBindVertexArray(lb.VAO);

    GLint primitive_count = 0;
    for (const LineBatch& batch : lb.Batches) {
        SetVec4(ps, shader, "uColor", batch.Color);
        glLineWidth(batch.LineWidth);

        glDrawArrays(batch.Mode, primitive_count, batch.PrimitiveCount);
        primitive_count += batch.PrimitiveCount;
    }

    glLineWidth(current_line_width);
}

LineBatcher* CreateLineBatcher(PlatformState* ps, LineBatcherRegistry* registry, const char* name) {
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
        .Name = InternString(&ps->Memory.StringArena, name),
        .VAO = vao,
        .VBO = vbo,
    };
    lb.Data.reserve(128);

    registry->LineBatchers[registry->Count] = std::move(lb);
    registry->Count++;

    return &registry->LineBatchers[registry->Count - 1];
}

LineBatcher* FindLineBatcher(LineBatcherRegistry* registry, const char* name) {
    for (u32 i = 0; i < registry->Count; i++) {
        auto& lb = registry->LineBatchers[i];
        // TODO(cdc): Use a better mechanism than searching for strings.
        if (strcmp(lb.Name, name) == 0) {
            return &lb;
        }
    }

    return nullptr;
}

// Mesh --------------------------------------------------------------------------------------------

void Bind(PlatformState*, const Mesh& mesh) {
    assert(IsValid(mesh));
    glBindVertexArray(mesh.VAO);
}

Mesh* CreateMesh(PlatformState* ps,
                 MeshRegistry* registry,
                 const char* name,
                 const CreateMeshOptions& options) {
    if (options.Vertices.empty()) {
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
                 options.Vertices.size_bytes(),
                 options.Vertices.data(),
                 options.MemoryUsage);

    if (!options.Indices.empty()) {
        GLuint ebo = GL_NONE;
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     options.Indices.size_bytes(),
                     options.Indices.data(),
                     options.MemoryUsage);
    }

    // Calculate stride (if needed).
    GLsizei stride = options.Stride;
    if (stride == 0) {
        for (u8 ap : options.AttribPointers) {
            stride += ap;
        }
        stride *= sizeof(float);
    }

    // Go over each attribute pointer.
    u64 offset = 0;
    for (u32 i = 0; i < options.AttribPointers.size(); i++) {
        // We iterate until we find a zero attribute pointer.
        u8 ap = options.AttribPointers[i];
        if (ap == 0) {
            break;
        }

        glVertexAttribPointer(i, ap, GL_FLOAT, GL_FALSE, stride, (void*)offset);
        glEnableVertexAttribArray(i);

        offset += ap * sizeof(float);
    }

    glBindVertexArray(GL_NONE);

    Mesh mesh{
        .Name = InternString(&ps->Memory.StringArena, name),
        .VAO = vao,
    };

    registry->Meshes[registry->Count] = std::move(mesh);
    registry->Count++;

    return &registry->Meshes[registry->Count - 1];
}

Mesh* FindMesh(MeshRegistry* registry, const char* name) {
    for (u32 i = 0; i < registry->Count; i++) {
        auto& mesh = registry->Meshes[i];
        // TODO(cdc): Use a better mechanism than searching for strings.
        if (strcmp(mesh.Name, name) == 0) {
            return &mesh;
        }
    }

    return nullptr;
}

// Shader ------------------------------------------------------------------------------------------

void Use(PlatformState*, const Shader& shader) {
    assert(IsValid(shader));
    glUseProgram(shader.Program);
}

void SetBool(PlatformState*, const Shader& shader, const char* uniform, bool value) {
    assert(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform1i(location, static_cast<i32>(value));
}

void SetI32(PlatformState*, const Shader& shader, const char* uniform, i32 value) {
    assert(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform1i(location, value);
}

void SetU32(PlatformState*, const Shader& shader, const char* uniform, u32 value) {
    assert(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform1ui(location, value);
}

void SetFloat(PlatformState*, const Shader& shader, const char* uniform, float value) {
    assert(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform1f(location, value);
}

void SetVec3(PlatformState*, const Shader& shader, const char* uniform, const glm::vec3& value) {
    assert(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform3f(location, value[0], value[1], value[2]);
}

void SetVec4(PlatformState*, const Shader& shader, const char* uniform, const glm::vec4& value) {
    assert(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        return;
    }
    glUniform4f(location, value[0], value[1], value[2], value[3]);
}

void SetMat4(PlatformState*, const Shader& shader, const char* uniform, const float* value) {
    assert(IsValid(shader));
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

Shader CreateNewShader(PlatformState* ps,
                       const char* name,
                       const char* vert_source,
                       const char* frag_source) {
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
        .Name = InternString(&ps->Memory.StringArena, name),
        .Program = program,
    };

    bool ok = SDL_GetCurrentTime(&shader.LastLoadTime);
    assert(ok);

    return shader;
}

}  // namespace opengl_private

Shader* CreateShader(PlatformState* ps,
                     ShaderRegistry* registry,
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

    Shader* shader = CreateShaderFromString(ps,
                                            registry,
                                            name,
                                            static_cast<const char*>(vert_source),
                                            static_cast<const char*>(frag_source));
    shader->VertPath = vert_path;
    shader->FragPath = frag_path;

    return shader;
}

Shader* CreateShaderFromString(PlatformState* ps,
                               ShaderRegistry* registry,
                               const char* name,
                               const char* vert_source,
                               const char* frag_source) {
    using namespace opengl_private;

    Shader shader = CreateNewShader(ps, name, vert_source, frag_source);
    registry->Shaders[registry->Count] = std::move(shader);
    registry->Count++;

    return &registry->Shaders[registry->Count - 1];
}

Shader* FindShader(ShaderRegistry* registry, const char* name) {
    for (u32 i = 0; i < registry->Count; i++) {
        auto& shader = registry->Shaders[i];
        // TODO(cdc): Use a better mechanism than searching for strings.
        if (strcmp(shader.Name, name) == 0) {
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
bool ReevaluateShader(PlatformState* ps, Shader* shader) {
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
        CreateNewShader(ps, shader->Name, (const char*)vert_source, (const char*)frag_source);
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

bool ReevaluateShaders(PlatformState* ps, ShaderRegistry* registry) {
    using namespace opengl_private;

    for (u32 i = 0; i < registry->Count; i++) {
        Shader& shader = registry->Shaders[i];
        if (!ReevaluateShader(ps, &shader)) {
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

void Bind(PlatformState*, const Texture& texture, GLuint texture_unit) {
    assert(IsValid(texture));
    glActiveTexture(texture_unit);
    glBindTexture(GL_TEXTURE_2D, texture.Handle);
}

Texture* CreateTexture(PlatformState* ps,
                       TextureRegistry* registry,
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
        case 3:
            format = GL_RGB;
            break;
        case 4:
            format = GL_RGBA;
            break;
        default:
            SDL_Log("ERROR: Unsupported number of channels: %d", channels);
            return nullptr;
            break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    Texture texture{
        .Name = InternString(&ps->Memory.StringArena, name),
        .Width = width,
        .Height = height,
        .Handle = handle,
    };

    registry->Textures[registry->Count] = std::move(texture);
    registry->Count++;

    return &registry->Textures[registry->Count - 1];
}

Texture* FindTexture(TextureRegistry* registry, const char* name) {
    for (u32 i = 0; i < registry->Count; i++) {
        auto& texture = registry->Textures[i];
        // TODO(cdc): Use a better mechanism than searching for strings.
        if (strcmp(texture.Name, name) == 0) {
            return &texture;
        }
    }

    return nullptr;
}
}  // namespace kdk
