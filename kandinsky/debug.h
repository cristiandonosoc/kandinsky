#pragma once

#include <kandinsky/color.h>
#include <kandinsky/defines.h>

#include <span>

namespace kdk {

struct Shader;

struct Debug {
    static bool Init();
    static void Shutdown();

    static void StartFrame();
    static void Render(const Shader& shader, const glm::mat4& view_proj);

    static void DrawLines(std::span<std::pair<glm::vec3, glm::vec3>> points, Color32 color,
                          float line_width = 1.0f);
    static void DrawArrow(const glm::vec3& start, const glm::vec3& end, Color32 color,
                          float arrow_size = 0.05f, float line_width = 1.0f);
    static void DrawSphere(const glm::vec3& center, float radius, u32 segments, Color32 color,
                           float line_width = 1.0f);
};

}  // namespace kdk
