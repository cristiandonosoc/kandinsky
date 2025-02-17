#pragma once

#include <kandinsky/color.h>
#include <kandinsky/defines.h>
#include <kandinsky/platform.h>

#include <span>

namespace kdk {

struct Shader;

struct Debug {
    static bool Init(PlatformState* ps);
    static void Shutdown(PlatformState* ps);

    static void StartFrame(PlatformState* ps);
    static void Render(PlatformState* ps, const Shader& shader, const glm::mat4& view_proj);

    static void DrawLines(PlatformState* ps,
                          std::span<std::pair<glm::vec3, glm::vec3>> points,
                          Color32 color,
                          float line_width = 1.0f);
    static void DrawArrow(PlatformState* ps,
                          const glm::vec3& start,
                          const glm::vec3& end,
                          Color32 color,
                          float arrow_size = 0.05f,
                          float line_width = 1.0f);
    static void DrawSphere(PlatformState* ps,
                           const glm::vec3& center,
                           float radius,
                           u32 segments,
                           Color32 color,
                           float line_width = 1.0f);
    static void DrawCone(PlatformState* ps,
                         const glm::vec3& origin,
                         const glm::vec3& direction,
                         float length,
                         float angle,
                         u32 segments,
                         Color32 color,
                         float line_width = 1.0f);
};

}  // namespace kdk
