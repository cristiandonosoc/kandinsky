#include <kandinsky/graphics/shader.h>

#include <kandinsky/core/file.h>
#include <kandinsky/core/memory.h>
#include <kandinsky/platform.h>

#include <SDL3/SDL_Log.h>

namespace kdk {

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

ShaderAssetHandle CreateShader(AssetRegistry* assets,
                               String asset_path,
                               const CreateShaderParams& params) {
    using namespace opengl_private;

    if (ShaderAssetHandle found = FindShaderHandle(assets, asset_path); IsValid(found)) {
        return found;
    }

    ASSERT(!assets->ShaderHolder.IsFull());

    auto scoped_arena = assets->AssetLoadingArena->GetScopedArena();
    String full_asset_path = GetFullAssetPath(scoped_arena, assets, asset_path);

    auto source = LoadFile(scoped_arena, full_asset_path);
    if (source.empty()) {
        SDL_Log("ERROR: reading shader at %s: %s\n", full_asset_path.Str(), SDL_GetError());
        return {};
    }

    // Create the shader.
    i32 asset_id = GenerateAssetID(EAssetType::Texture, asset_path);
    Shader shader = CreateNewShader(asset_id, full_asset_path, String(source));
    if (!IsValid(shader)) {
        return {};
    }

    SDL_Log("Created shader %s\n", asset_path.Str());
    AssetHandle handle = assets->ShaderHolder.PushAsset(asset_id,
                                                        asset_path,
                                                        params.AssetOptions,
                                                        std::move(shader));
    return {handle};
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

bool ReevaluateShaders(AssetRegistry* assets) {
    using namespace opengl_private;

    // TODO(cdc): Write an iterator for this, rather than relying on both asset and underlying asset
    //            being always in sync.
    for (i32 i = 0; i < assets->ShaderHolder.Assets.Size; i++) {
        if (!IsValid(assets->ShaderHolder.Assets[i])) {
            continue;
        }

        Shader* shader = &assets->ShaderHolder.UnderlyingAssets[i];
        ASSERT(IsValid(*shader));

        if (!ReevaluateShader(shader)) {
            SDL_Log("ERROR: Re-evaluating shader %d: %s", i, shader->Path.Str());
            return true;
        }
    }

    return true;
}

}  // namespace kdk
