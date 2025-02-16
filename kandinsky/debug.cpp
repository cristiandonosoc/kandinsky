#include <kandinsky/debug.h>

#include <kandinsky/opengl.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace kdk {

namespace debug_private {

LineBatcher gDebugLineBatcher = {};

}  // namespace debug_private

bool Debug::Init() {
    assert(!IsValid(debug_private::gDebugLineBatcher));
    debug_private::gDebugLineBatcher = CreateLineBatcher();

    return true;
}

void Debug::Shutdown() {
    assert(IsValid(debug_private::gDebugLineBatcher));
    debug_private::gDebugLineBatcher = {};
}

void Debug::StartFrame() {
    assert(IsValid(debug_private::gDebugLineBatcher));

    Reset(&debug_private::gDebugLineBatcher);
}

void Debug::Render(const Shader& shader, const glm::mat4& view_proj) {
    // TODO(cdc): Maybe detect differences so that we don't send redundant data every frame.
    Buffer(debug_private::gDebugLineBatcher);

    Use(shader);
    SetMat4(shader, "uViewProj", glm::value_ptr(view_proj));
    Draw(shader, debug_private::gDebugLineBatcher);
}

void Debug::DrawSphere(const glm::vec3& center, float radius, u32 segments, Color32 color,
                       float line_width) {
    using namespace debug_private;

    // Need at least 4 segments
    segments = glm::max(segments, (u32)4);

    const float angle_inc = 2.f * PI / segments;
    u32 num_segments_y = segments / 2;
    float latitude = angle_inc;
    float siny1 = 0.0f, cosy1 = 1.0f;

    StartLineBatch(&gDebugLineBatcher, GL_LINES, color, line_width);

    while (num_segments_y--) {
        const float siny2 = glm::sin(latitude);
        const float cosy2 = glm::cos(latitude);

        glm::vec3 vertex1 = glm::vec3(siny1, cosy1, 0.0f) * radius + center;
        glm::vec3 vertex3 = glm::vec3(siny2, cosy2, 0.0f) * radius + center;
        float longitude = angle_inc;

        u32 num_segments_x = segments;
        while (num_segments_x--) {
            const float sinx = glm::sin(longitude);
            const float cosx = glm::cos(longitude);

            glm::vec3 vertex2 = glm::vec3((cosx * siny1), cosy1, (sinx * siny1)) * radius + center;
            glm::vec3 vertex4 = glm::vec3((cosx * siny2), cosy2, (sinx * siny2)) * radius + center;

            AddPoint(&gDebugLineBatcher, vertex1);
            AddPoint(&gDebugLineBatcher, vertex2);

            AddPoint(&gDebugLineBatcher, vertex1);
            AddPoint(&gDebugLineBatcher, vertex3);

            vertex1 = vertex2;
            vertex3 = vertex4;
            longitude += angle_inc;
        }
        siny1 = siny2;
        cosy1 = cosy2;
        latitude += angle_inc;
    }

    EndLineBatch(&gDebugLineBatcher);
}

}  // namespace kdk
