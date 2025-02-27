#include <kandinsky/glew.h>

#include <kandinsky/platform.h>
#include <kandinsky/print.h>

#include <GL/glew.h>
#include <SDL3/SDL.h>

namespace kdk {

namespace glew_private {

// Callback function for printing debug statements
void GLDebugMessageCallback(GLenum source,
                            GLenum type,
                            GLuint id,
                            GLenum severity,
                            GLsizei length,
                            const GLchar* msg,
                            const void* data) {
    (void)length;
    PlatformState* ps = (PlatformState*)data;

    const char* source_str;
    const char* type_str;
    const char* severity_str;

    switch (source) {
        case GL_DEBUG_SOURCE_API: source_str = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: source_str = "WINDOW SYSTEM"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: source_str = "SHADER COMPILER"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: source_str = "THIRD PARTY"; break;
        case GL_DEBUG_SOURCE_APPLICATION: source_str = "APPLICATION"; break;
        case GL_DEBUG_SOURCE_OTHER: source_str = "UNKNOWN"; break;
        default: source_str = "UNKNOWN"; break;
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR: type_str = "ERROR"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_str = "DEPRECATED BEHAVIOR"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: type_str = "UDEFINED BEHAVIOR"; break;
        case GL_DEBUG_TYPE_PORTABILITY: type_str = "PORTABILITY"; break;
        case GL_DEBUG_TYPE_PERFORMANCE: type_str = "PERFORMANCE"; break;
        case GL_DEBUG_TYPE_OTHER: type_str = "OTHER"; break;
        case GL_DEBUG_TYPE_MARKER: type_str = "MARKER"; break;
        default: type_str = "UNKNOWN"; break;
    }

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH: severity_str = "HIGH"; break;
        case GL_DEBUG_SEVERITY_MEDIUM: severity_str = "MEDIUM"; break;
        case GL_DEBUG_SEVERITY_LOW: severity_str = "LOW"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            // Ignore notifications.
            return;
        default: severity_str = "UNKNOWN"; break;
    }

    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        SDL_Log("GL_ERROR: %d: %s of %s severity, raised from %s: %s\n",
                id,
                type_str,
                severity_str,
                source_str,
                msg);
        PrintBacktrace(&ps->Memory.FrameArena, 1);
        std::abort();
    }
}

}  // namespace glew_private

bool InitGlew(PlatformState* ps) {
    // Initialize GLEW.
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        SDL_Log("GLEW Init error: %s\n", glewGetErrorString(glewError));
        return false;
    }

    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    ASSERT(glGetError() == GL_NO_ERROR);

    glDebugMessageCallback(glew_private::GLDebugMessageCallback, ps);

    return true;
}

}  // namespace kdk
