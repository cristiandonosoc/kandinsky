#pragma once

#include <kandinsky/defines.h>

// Simple header that simplifies adding glm stuff.
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace kdk {

struct Math {
    static inline bool Equals(float a, float b, float tolerance = KINDA_SMALL_NUMBER) {
        return glm::abs(b - a) <= tolerance;
    }

    static inline bool Equals(double a, double b, double tolerance = KINDA_SMALL_NUMBER) {
        return glm::abs(b - a) <= tolerance;
    }
};

}  // namespace kdk
