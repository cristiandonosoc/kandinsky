#pragma once

#include <kandinsky/core/color.h>
#include <kandinsky/core/defines.h>
#include <kandinsky/platform.h>

#include <span>

namespace kdk {

struct Shader;

struct Debug {
    static bool Init(PlatformState* ps);
    static void Shutdown(PlatformState* ps);

    static void StartFrame(PlatformState* ps);
    static void Render(PlatformState* ps, const Shader& shader, const Mat4& view_proj);

    static void DrawLines(PlatformState* ps,
                          std::span<std::pair<Vec3, Vec3>> points,
                          Color32 color,
                          float line_width = 1.0f);

    static void DrawBox(PlatformState* ps,
                        const Vec3& center,
                        const Vec3& extents,
                        Color32 color,
                        float line_width = 1.0f);

    static void DrawArrow(PlatformState* ps,
                          const Vec3& start,
                          const Vec3& end,
                          Color32 color,
                          float arrow_size = 0.05f,
                          float line_width = 1.0f);

    static void DrawSphere(PlatformState* ps,
                           const Vec3& center,
                           float radius,
                           u32 segments,
                           Color32 color,
                           float line_width = 1.0f);

    static void DrawCone(PlatformState* ps,
                         const Vec3& origin,
                         const Vec3& direction,
                         float length,
                         float angle,
                         u32 segments,
                         Color32 color,
                         float line_width = 1.0f);

    static void DrawFrustum(PlatformState* ps,
                            const Mat4& M_ViewProj,
                            Color32 color,
                            float line_width = 1.0f);
};

}  // namespace kdk
