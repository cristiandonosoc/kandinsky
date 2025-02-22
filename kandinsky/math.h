#pragma once

#include <kandinsky/defines.h>

// Simple header that simplifies adding glm stuff.
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace kdk {

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat4 = glm::mat4;

template <typename T>
auto* GetPtr(T& t) {
    return glm::value_ptr(t);
}

struct Math {
    static inline bool Equals(float a, float b, float tolerance = KINDA_SMALL_NUMBER) {
        return glm::abs(b - a) <= tolerance;
    }

    static inline bool Equals(double a, double b, double tolerance = KINDA_SMALL_NUMBER) {
        return glm::abs(b - a) <= tolerance;
    }
};

}  // namespace kdk
