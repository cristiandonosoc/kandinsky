#include <kandinsky/debug.h>

#include <kandinsky/graphics/opengl.h>
#include <kandinsky/math.h>

#include <SDL3/SDL_log.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace kdk {

bool Debug::Init(PlatformState* ps) {
    if (ps->DebugLineBatcher) {
        SDL_Log("ERROR: DebugLineBatcher already set");
        return false;
    }

    if (FindLineBatcher(&ps->LineBatchers, "DebugLineBatcher")) {
        SDL_Log("ERROR: DebugLineBatcher already found");
        return false;
    }

    ps->DebugLineBatcher = CreateLineBatcher(&ps->LineBatchers, "DebugLineBatcher");

    return true;
}

void Debug::Shutdown(PlatformState* ps) {
    ASSERT(IsValid(*ps->DebugLineBatcher));
    ps->DebugLineBatcher = nullptr;
}

void Debug::StartFrame(PlatformState* ps) {
    ASSERT(IsValid(*ps->DebugLineBatcher));

    Reset(ps->DebugLineBatcher);
}

void Debug::Render(PlatformState* ps, const Shader& shader, const Mat4& view_proj) {
    // TODO(cdc): Maybe detect differences so that we don't send redundant data every frame.
    Buffer(ps, *ps->DebugLineBatcher);

    Use(shader);
    SetMat4(shader, "uM_ViewProj", GetPtr(view_proj));
    Draw(*ps->DebugLineBatcher, shader);
}

void Debug::DrawLines(PlatformState* ps,
                      std::span<std::pair<Vec3, Vec3>> points,
                      Color32 color,
                      float line_width) {
    StartLineBatch(ps->DebugLineBatcher, GL_LINES, color, line_width);

    for (const auto& [p1, p2] : points) {
        AddPoints(ps->DebugLineBatcher, p1, p2);
    }

    EndLineBatch(ps->DebugLineBatcher);
}

void Debug::DrawArrow(PlatformState* ps,
                      const Vec3& start,
                      const Vec3& end,
                      Color32 color,
                      float arrow_size,
                      float line_width) {
    // Find the axis vectors.
    Vec3 direction = Normalize(end - start);
    Vec3 up(0, 1, 0);
    Vec3 right = Normalize(Cross(up, direction));
    up = Normalize(Cross(direction, right));

    Vec3 front_offset = direction * arrow_size;
    Vec3 right_offset = right * arrow_size;
    Vec3 up_offset = up * arrow_size;

    StartLineBatch(ps->DebugLineBatcher, GL_LINES, color, line_width);
    AddPoints(ps->DebugLineBatcher, start, end);

    AddPoints(ps->DebugLineBatcher, end, end - front_offset - right_offset - up_offset);
    AddPoints(ps->DebugLineBatcher, end, end - front_offset - right_offset + up_offset);
    AddPoints(ps->DebugLineBatcher, end, end - front_offset + right_offset - up_offset);
    AddPoints(ps->DebugLineBatcher, end, end - front_offset + right_offset + up_offset);

    EndLineBatch(ps->DebugLineBatcher);
}

void Debug::DrawSphere(PlatformState* ps,
                       const Vec3& center,
                       float radius,
                       u32 segments,
                       Color32 color,
                       float line_width) {
    // Need at least 4 segments
    segments = Max(segments, (u32)4);

    const float angle_inc = 2.f * PI / segments;
    u32 num_segments_y = segments / 2;
    float latitude = angle_inc;
    float siny1 = 0.0f, cosy1 = 1.0f;

    StartLineBatch(ps->DebugLineBatcher, GL_LINES, color, line_width);

    while (num_segments_y--) {
        const float siny2 = Sin(latitude);
        const float cosy2 = Cos(latitude);

        Vec3 vertex1 = Vec3(siny1, cosy1, 0.0f) * radius + center;
        Vec3 vertex3 = Vec3(siny2, cosy2, 0.0f) * radius + center;
        float longitude = angle_inc;

        u32 num_segments_x = segments;
        while (num_segments_x--) {
            const float sinx = Sin(longitude);
            const float cosx = Cos(longitude);

            Vec3 vertex2 = Vec3((cosx * siny1), cosy1, (sinx * siny1)) * radius + center;
            Vec3 vertex4 = Vec3((cosx * siny2), cosy2, (sinx * siny2)) * radius + center;

            AddPoints(ps->DebugLineBatcher, vertex1, vertex2);
            AddPoints(ps->DebugLineBatcher, vertex1, vertex3);

            vertex1 = vertex2;
            vertex3 = vertex4;
            longitude += angle_inc;
        }
        siny1 = siny2;
        cosy1 = cosy2;
        latitude += angle_inc;
    }

    EndLineBatch(ps->DebugLineBatcher);
}

void Debug::DrawCone(PlatformState* ps,
                     const Vec3& origin,
                     const Vec3& direction,
                     float length,
                     float angle,
                     u32 segments,
                     Color32 color,
                     float line_width) {
    const Vec3 front = Normalize(direction);

    // If the cone is looking exactly up or down (which can be common), we change the up axis to
    // look "up" the x axis.
    Vec3 up(0, 1, 0);
    if (Math::Equals(Abs(Dot(front, up)), 1.0f)) {
        up = {1, 0, 0};
    }

    const Vec3 end = origin + front * length;

    // Determine the axis.
    Vec3 right = Normalize(Cross(up, front));
    up = Normalize(Cross(front, right));

    // Need at least 4 sides
    segments = Max(segments, (u32)4);
    u32 num_segments = segments;

    const float radius = length * Tan(angle);
    const Vec3 rright = right * radius;
    const Vec3 rup = up * radius;

    float sin1 = 0.0f;
    float cos1 = 1.0f;

    // TODO(cdc): This could be improved to use GL_LINE_STRIP for the circle.
    StartLineBatch(ps->DebugLineBatcher, GL_LINES, color, line_width);

    const float circle_angle_inc = 2.0f * PI / segments;
    float circle_angle = circle_angle_inc;
    while (num_segments--) {
        const float sin2 = Sin(circle_angle);
        const float cos2 = Cos(circle_angle);

        Vec3 v1 = end + rright * sin1 + rup * cos1;
        Vec3 v2 = end + rright * sin2 + rup * cos2;

        AddPoints(ps->DebugLineBatcher, v1, v2);
        AddPoints(ps->DebugLineBatcher, origin, v2);

        sin1 = sin2;
        cos1 = cos2;
        circle_angle += circle_angle_inc;
    }

    EndLineBatch(ps->DebugLineBatcher);
}

void Debug::DrawFrustum(PlatformState* ps,
                        const Mat4& M_ViewProj,
                        Color32 color,
                        float line_width) {
    // First, get the inverse of projection-view matrix to transform from clip space to world space
    Mat4 M_InverseViewProj = Inverse(M_ViewProj);

    // Define the 8 corners of the frustum in normalized device coordinates (NDC)
    // NDC cube corners go from (-1,-1,-1) to (1,1,1)
    Vec3 corners[8];

    // clang-format off
    // Near plane corners (z = -1 in NDC)
    corners[0] = TransformPoint(M_InverseViewProj, Vec3(-1, -1, -1));	// bottom-left-near
    corners[1] = TransformPoint(M_InverseViewProj, Vec3( 1, -1, -1));   // bottom-right-near
    corners[2] = TransformPoint(M_InverseViewProj, Vec3( 1,  1, -1));	// top-right-near
    corners[3] = TransformPoint(M_InverseViewProj, Vec3(-1,  1, -1));   // top-left-near

    // Far plane corners (z = 1 in NDC)
    corners[4] = TransformPoint(M_InverseViewProj, Vec3(-1, -1,  1));	// bottom-left-far
    corners[5] = TransformPoint(M_InverseViewProj, Vec3( 1, -1,  1));   // bottom-right-far
    corners[6] = TransformPoint(M_InverseViewProj, Vec3( 1,  1,  1));   // top-right-far
    corners[7] = TransformPoint(M_InverseViewProj, Vec3(-1,  1,  1));   // top-left-far
                                                                     // clang-format on

    // Start batch for drawing lines
    StartLineBatch(ps->DebugLineBatcher, GL_LINES, color, line_width);

    // Draw near plane
    AddPoints(ps->DebugLineBatcher, corners[0], corners[1]);
    AddPoints(ps->DebugLineBatcher, corners[1], corners[2]);
    AddPoints(ps->DebugLineBatcher, corners[2], corners[3]);
    AddPoints(ps->DebugLineBatcher, corners[3], corners[0]);

    // Draw far plane
    AddPoints(ps->DebugLineBatcher, corners[4], corners[5]);
    AddPoints(ps->DebugLineBatcher, corners[5], corners[6]);
    AddPoints(ps->DebugLineBatcher, corners[6], corners[7]);
    AddPoints(ps->DebugLineBatcher, corners[7], corners[4]);

    // Connect near and far planes
    AddPoints(ps->DebugLineBatcher, corners[0], corners[4]);
    AddPoints(ps->DebugLineBatcher, corners[1], corners[5]);
    AddPoints(ps->DebugLineBatcher, corners[2], corners[6]);
    AddPoints(ps->DebugLineBatcher, corners[3], corners[7]);

    EndLineBatch(ps->DebugLineBatcher);
}

}  // namespace kdk
