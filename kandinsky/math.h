#pragma once

#include <kandinsky/defines.h>

// Simple header that simplifies adding glm stuff.
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace kdk {

struct Arena;

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat4 = glm::mat4;

const char* ToString(Arena* arena, const Vec2 v);
const char* ToString(Arena* arena, const Vec3 v);
const char* ToString(Arena* arena, const Vec4 v);

template <typename T>
auto* GetPtr(T& t) {
    return glm::value_ptr(t);
}

inline float Abs(float v) { return glm::abs(v); }

template <typename T>
inline T Min(T v1, T v2) { return glm::min(v1, v2); }
template <typename T>
inline T Max(T v1, T v2) { return glm::max(v1, v2); }

inline Vec3 Normalize(const Vec3& v) { return glm::normalize(v); }

inline Mat4 Translate(const Mat4& m, const Vec3& pos) { return glm::translate(m, pos); }
inline Mat4 Rotate(const Mat4& m, float angle, const Vec3& axis) {
	return glm::rotate(m, angle, axis);
}
inline Mat4 Scale(const Mat4& m, const Vec3& pos) { return glm::scale(m, pos); }
inline float Distance(const Vec3& v1, const Vec3& v2) { return glm::distance(v1, v2); }

inline constexpr float ToRadians(float deg) { return glm::radians(deg); }

inline Mat4 LookAt(const Vec3& pos, const Vec3& front, const Vec3& up) {
    return glm::lookAt(pos, front, up);
}

inline Mat4 Perspective(float fovy, float aspect, float znear, float zfar) {
    return glm::perspective(fovy, aspect, znear, zfar);
}

inline float Dot(const Vec3& v1, const Vec3& v2) { return glm::dot(v1, v2); }
inline Vec3 Cross(const Vec3& v1, const Vec3& v2) { return glm::cross(v1, v2); }

inline float FMod(float v, float limit) { return glm::mod(v, limit); }

inline float Clamp(float v, float min, float max) { return glm::clamp(v, min, max); }

inline Mat4 Inverse(const Mat4& m) { return glm::inverse(m); }
inline Mat4 Transpose(const Mat4& m) { return glm::transpose(m); }

inline float Cos(float angle) { return glm::cos(angle); }
inline float Sin(float angle) { return glm::sin(angle); }
inline float Tan(float angle) { return glm::tan(angle); }

struct Math {
    static inline bool Equals(float a, float b, float tolerance = KINDA_SMALL_NUMBER) {
        return glm::abs(b - a) <= tolerance;
    }

    static inline bool Equals(double a, double b, double tolerance = KINDA_SMALL_NUMBER) {
        return glm::abs(b - a) <= tolerance;
    }
};

}  // namespace kdk
