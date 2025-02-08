#include <kandinsky/utils/defer.h>
#include <kandinsky/window.h>

static bool gDone = false;

static constexpr int kWidth = 1024;
static constexpr int kHeight = 720;

namespace GL {

void PrintProgramLog(GLuint program);
void PrintShaderLog(GLuint shader);

}  // namespace GL

// clang-format off
constexpr float kVertices[] = {
	 -0.5f, -0.5f, 0.0f,
      0.5f, -0.5f, 0.0f,
      0.0f,  0.5f, 0.0f
};
// clang-format on

const char* kVertexShaderSource = R"%(
#version 330 core
layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)%";

const char* kFragmentShaderSource = R"%(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
)%";

struct ShaderState {
    GLuint Program = GL_NONE;
    GLuint VAO = GL_NONE;
};
ShaderState gShaderState = {};

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

GLuint CompileProgram() {
    GLuint vs = CompileShader(GL_VERTEX_SHADER, kVertexShaderSource);
    if (vs == GL_NONE) {
        SDL_Log("ERROR: Compiling vertex shader");
        return GL_NONE;
    }
    DEFER { glDeleteShader(vs); };

    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);
    if (fs == GL_NONE) {
        SDL_Log("ERROR: Compiling fragment shader");
        return GL_NONE;
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
        return GL_NONE;
    }

    return program;
}

bool InitRender() {
    // Bind the Vertex Array Object (VAO).
    GLuint vao = GL_NONE;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Copy our vertices into a Vertex Buffer Object (VBO).
    GLuint vbo = GL_NONE;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

    // Set the attributes.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    GLuint program = CompileProgram();
    if (program == GL_NONE) {
        return false;
    }

    gShaderState = {
        .Program = program,
        .VAO = vao,
    };

    return true;
}

void Render() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(gShaderState.Program);
	glBindVertexArray(gShaderState.VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);

}

bool Update() {
    using namespace kdk;

    if (!PollWindowEvents()) {
        return false;
    }

    Render();

    SDL_Delay(1);
    return true;
}

int main() {
    using namespace kdk;

    if (!InitWindow(kWidth, kHeight)) {
        return -1;
    }
    DEFER { ShutdownWindow(); };

    if (!InitRender()) {
        return -1;
    }

    glViewport(0, 0, kWidth, kHeight);

    // Use VSync.
    if (!SDL_GL_SetSwapInterval(1)) {
        SDL_Log("Unable to set VSYNC: %s\n", SDL_GetError());
        return -1;
    }

    while (!gDone) {
        if (!Update()) {
            break;
        }

        if (!SDL_GL_SwapWindow(gSDLWindow)) {
            SDL_Log("ERROR: Could not swap buffer: %s\n", SDL_GetError());
            return -1;
        }
    }

    return 0;
}
