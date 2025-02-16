#pragma once

#include <kandinsky/color.h>
#include <kandinsky/defines.h>

namespace kdk {

struct Shader;

struct Debug {
    static bool Init();
    static void Shutdown();

    static void StartFrame();
    static void Render(const Shader& shader, const glm::mat4& view_proj);

    static void DrawSphere(const glm::vec3& center, float radius, u32 segments, Color32 color,
                           float line_width = 1.0f);
};

}  // namespace kdk
