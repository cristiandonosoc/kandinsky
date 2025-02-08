#include <SDL3/SDL_iostream.h>
#include <kandinsky/opengl.h>

#include <SDL3/SDL.h>
#include <kandinsky/utils/defer.h>
#include <cassert>

namespace kdk {
namespace opengl_private {

GLuint CompileShader(GLuint shader_type, const char* source) {
    unsigned int handle = glCreateShader(shader_type);
    glShaderSource(handle, 1, &source, NULL);
    glCompileShader(handle);

    int success = 0;
    char log[512];

    glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(handle, sizeof(log), NULL, log);
        SDL_Log("ERROR: Compiling shader: %s\n", log);
        return GL_NONE;
    }

    return handle;
}

}  // namespace opengl_private

Shader CreateShader(const char* vs_path, const char* fs_path) {
    void* vs_source = SDL_LoadFile(vs_path, nullptr);
    if (!vs_source) {
        SDL_Log("ERROR: reading vertex shader at %s\n", vs_path);
        return {};
    }
    DEFER { SDL_free(vs_source); };

    void* fs_source = SDL_LoadFile(fs_path, nullptr);
    if (!fs_source) {
        SDL_Log("ERROR: reading fragment shader at %s\n", fs_path);
        return {};
    }
    DEFER { SDL_free(fs_source); };

    Shader shader = CreateShaderFromString(static_cast<const char*>(vs_source), static_cast<const char*>(fs_source));
    if (!IsValid(shader)) {
        return {};
    }

    return shader;
}

Shader CreateShaderFromString(const char* vs_source, const char* fragment_source) {
    using namespace opengl_private;

    GLuint vs = CompileShader(GL_VERTEX_SHADER, vs_source);
    if (vs == GL_NONE) {
        SDL_Log("ERROR: Compiling vertex shader");
        return {};
    }
    DEFER { glDeleteShader(vs); };

    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragment_source);
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

    return Shader{
        .Program = program,
    };
}

void Use(const Shader& shader) {
    assert(IsValid(shader));
    glUseProgram(shader.Program);
}

void SetBool(const Shader& shader, const char* uniform, bool value) {
    assert(IsValid(shader));
    glUniform1i(glGetUniformLocation(shader.Program, uniform), static_cast<i32>(value));
}

void SetU32(const Shader& shader, const char* uniform, u32 value) {
    assert(IsValid(shader));
    glUniform1ui(glGetUniformLocation(shader.Program, uniform), value);
}

void SetFloat(const Shader& shader, const char* uniform, float value) {
    assert(IsValid(shader));
    glUniform1f(glGetUniformLocation(shader.Program, uniform), value);
}

}  // namespace kdk
