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

void Debug::DrawLines(std::span<std::pair<glm::vec3, glm::vec3>> points, Color32 color,
                      float line_width) {
    StartLineBatch(&debug_private::gDebugLineBatcher, GL_LINES, color, line_width);

    for (const auto& [p1, p2] : points) {
        AddPoints(&debug_private::gDebugLineBatcher, p1, p2);
    }

    EndLineBatch(&debug_private::gDebugLineBatcher);
}

void Debug::DrawArrow(const glm::vec3& start, const glm::vec3& end, Color32 color, float arrow_size,
                      float line_width) {
    using namespace debug_private;

    // Find the axis vectors.
    glm::vec3 front = glm::normalize(end - start);

    glm::vec3 up(0, 1, 0);
    glm::vec3 right = glm::normalize(glm::cross(up, front));
    up = glm::normalize(glm::cross(front, right));

    glm::vec3 front_offset = front * arrow_size;
    glm::vec3 right_offset = right * arrow_size;
    glm::vec3 up_offset = up * arrow_size;

    StartLineBatch(&debug_private::gDebugLineBatcher, GL_LINES, color, line_width);
    AddPoints(&gDebugLineBatcher, start, end);

    AddPoints(&gDebugLineBatcher, end, end - front_offset - right_offset - up_offset);
    AddPoints(&gDebugLineBatcher, end, end - front_offset - right_offset + up_offset);
    AddPoints(&gDebugLineBatcher, end, end - front_offset + right_offset - up_offset);
    AddPoints(&gDebugLineBatcher, end, end - front_offset + right_offset + up_offset);

    EndLineBatch(&debug_private::gDebugLineBatcher);
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

            AddPoints(&gDebugLineBatcher, vertex1, vertex2);
            AddPoints(&gDebugLineBatcher, vertex1, vertex3);

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
