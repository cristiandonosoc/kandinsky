#pragma once

#include <kandinsky/game.h>
#include <kandinsky/color.h>
#include <kandinsky/defines.h>

#include <span>

namespace kdk {

struct Shader;

struct Debug {
    static bool Init(PlatformState* platform_state);
    static void Shutdown(PlatformState* platform_state);

    static void StartFrame();
    static void Render(const Shader& shader, const glm::mat4& view_proj);

    static void DrawLines(std::span<std::pair<glm::vec3, glm::vec3>> points,
                          Color32 color,
                          float line_width = 1.0f);
    static void DrawArrow(const glm::vec3& start,
                          const glm::vec3& end,
                          Color32 color,
                          float arrow_size = 0.05f,
                          float line_width = 1.0f);
    static void DrawSphere(const glm::vec3& center,
                           float radius,
                           u32 segments,
                           Color32 color,
                           float line_width = 1.0f);
    static void DrawCone(const glm::vec3& origin,
                         const glm::vec3& direction,
                         float length,
                         float angle,
                         u32 segments,
                         Color32 color,
                         float line_width = 1.0f);
};

}  // namespace kdk
