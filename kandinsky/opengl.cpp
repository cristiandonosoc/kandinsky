#include <kandinsky/opengl.h>

#include <kandinsky/input.h>
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

void Update(Camera* camera, float dt) {
    constexpr float kMaxPitch = glm::radians(89.0f);

    if (MOUSE_PRESSED(MIDDLE)) {
        glm::vec2 offset = gInputState->MouseMove * camera->MouseSensitivity;

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

    float speed = camera->MovementSpeed * dt;
    if (KEY_PRESSED(W)) {
        camera->Position += speed * camera->Front;
    }
    if (KEY_PRESSED(S)) {
        camera->Position -= speed * camera->Front;
    }
    if (KEY_PRESSED(A)) {
        camera->Position -= speed * camera->Right;
    }
    if (KEY_PRESSED(D)) {
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

void Buffer(const LineBatcher& lb) {
    assert(IsValid(lb));

    // Send the data.
    glBindBuffer(GL_ARRAY_BUFFER, lb.VBO);
    glBufferData(GL_ARRAY_BUFFER, lb.Data.size(), lb.Data.data(), GL_STREAM_DRAW);
}

void Draw(const Shader& shader, const LineBatcher& lb) {
    assert(IsValid(lb));

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

    LineBatcher lb{
        .Name = name,
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

void Bind(const Mesh& mesh) {
    assert(IsValid(mesh));
    glBindVertexArray(mesh.VAO);
}

Mesh* CreateMesh(MeshRegistry* registry, const char* name, const CreateMeshOptions& options) {
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
        .Name = name,
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

}  // namespace opengl_private

void Use(const Shader& shader) {
    assert(IsValid(shader));
    glUseProgram(shader.Program);
}

void SetBool(const Shader& shader, const char* uniform, bool value) {
    assert(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        SDL_Log("ERROR: Shader %s: uniform %s not found", shader.Name, uniform);
        assert(false);
    }
    glUniform1i(location, static_cast<i32>(value));
}

void SetI32(const Shader& shader, const char* uniform, i32 value) {
    assert(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        SDL_Log("ERROR: Shader %s: uniform %s not found", shader.Name, uniform);
        assert(false);
    }
    glUniform1i(location, value);
}

void SetU32(const Shader& shader, const char* uniform, u32 value) {
    assert(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        SDL_Log("ERROR: Shader %s: uniform %s not found", shader.Name, uniform);
        assert(false);
    }
    glUniform1ui(location, value);
}

void SetFloat(const Shader& shader, const char* uniform, float value) {
    assert(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        SDL_Log("ERROR: Shader %s: uniform %s not found", shader.Name, uniform);
        assert(false);
    }
    glUniform1f(location, value);
}

void SetVec3(const Shader& shader, const char* uniform, const glm::vec3& value) {
    assert(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        SDL_Log("ERROR: Shader %s: uniform %s not found", shader.Name, uniform);
        assert(false);
    }
    glUniform3f(location, value[0], value[1], value[2]);
}

void SetVec4(const Shader& shader, const char* uniform, const glm::vec4& value) {
    assert(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        SDL_Log("ERROR: Shader %s: uniform %s not found", shader.Name, uniform);
        assert(false);
    }
    glUniform4f(location, value[0], value[1], value[2], value[3]);
}

void SetMat4(const Shader& shader, const char* uniform, const float* value) {
    assert(IsValid(shader));
    GLint location = glGetUniformLocation(shader.Program, uniform);
    if (location == -1) {
        SDL_Log("ERROR: Shader %s: uniform %s not found", shader.Name, uniform);
        assert(false);
    }
    glUniformMatrix4fv(location, 1, GL_FALSE, value);
}

Shader* CreateShader(ShaderRegistry* registry,
                     const char* name,
                     const char* vs_path,
                     const char* fs_path) {
    void* vs_source = SDL_LoadFile(vs_path, nullptr);
    if (!vs_source) {
        SDL_Log("ERROR: reading vertex shader at %s: %s\n", vs_path, SDL_GetError());
        return nullptr;
    }
    DEFER { SDL_free(vs_source); };

    void* fs_source = SDL_LoadFile(fs_path, nullptr);
    if (!fs_source) {
        SDL_Log("ERROR: reading fragment shader at %s: %s\n", fs_path, SDL_GetError());
        return nullptr;
    }
    DEFER { SDL_free(fs_source); };

    return CreateShaderFromString(registry,
                                  name,
                                  static_cast<const char*>(vs_source),
                                  static_cast<const char*>(fs_source));
}

Shader* CreateShaderFromString(ShaderRegistry* registry,
                               const char* name,
                               const char* vs_source,
                               const char* fragment_source) {
    using namespace opengl_private;

    GLuint vs = CompileShader(name, "vertex", GL_VERTEX_SHADER, vs_source);
    if (vs == GL_NONE) {
        SDL_Log("ERROR: Compiling vertex shader");
        return nullptr;
    }
    DEFER { glDeleteShader(vs); };

    GLuint fs = CompileShader(name, "fragment", GL_FRAGMENT_SHADER, fragment_source);
    if (fs == GL_NONE) {
        SDL_Log("ERROR: Compiling fragment shader");
        return nullptr;
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
        return nullptr;
    }

    Shader shader{
        .Name = name,
        .Program = program,
    };

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

// Texture -----------------------------------------------------------------------------------------

bool IsValid(const Texture& texture) {
    return texture.Width != 0 && texture.Height != 0 && texture.Handle != GL_NONE;
}

void Bind(const Texture& texture, GLuint texture_unit) {
    assert(IsValid(texture));
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
        .Name = name,
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
