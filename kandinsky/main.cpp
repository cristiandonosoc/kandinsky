#include <kandinsky/defines.h>
#include <kandinsky/utils/defer.h>
#include <kandinsky/window.h>
#include <string>

static bool gDone = false;

static constexpr int kWidth = 1024;
static constexpr int kHeight = 720;

namespace GL {

void PrintProgramLog(GLuint program);
void PrintShaderLog(GLuint shader);

}  // namespace GL

// clang-format off
float kVertices[] = {
    // positions         // colors
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right
    -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left
     0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // top
};

u32 kIndices[] = {
	0, 1, 3,
	1, 2, 3,
};
// clang-format on

std::string kBasePath;

struct ShaderState {
    kdk::Shader Shader = {};
    GLuint VAO = GL_NONE;
};
ShaderState gShaderState = {};

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

    GLuint ebo = GL_NONE;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices, GL_STATIC_DRAW);

    // Set the attributes.

    GLsizei stride = 6 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Unbind the VAO.
    glBindVertexArray(GL_NONE);

    std::string vs_path = kBasePath + "shaders/shader.vert";
    std::string fs_path = kBasePath + "shaders/shader.frag";
    auto shader = kdk::CreateShader(vs_path.c_str(), fs_path.c_str());
    if (!IsValid(shader)) {
        return false;
    }
    gShaderState = {
        .Shader = shader,
        .VAO = vao,
    };

    return true;
}

void Render() {
    float ms = static_cast<float>(SDL_GetTicks()) / 1000.0f;
    float green = (sin(ms) / 2.0f) + 0.5f;
    //GLint vertex_color_location = glGetUniformLocation(gShaderState.Shader.Program, "ourColor");

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

	Use(gShaderState.Shader);
    //glUniform4f(vertex_color_location, 0.0f, green, 0.0f, 1.0f);
	SetFloat(gShaderState.Shader, "green", green);

    glBindVertexArray(gShaderState.VAO);
    // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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

    kBasePath = SDL_GetCurrentDirectory();

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
