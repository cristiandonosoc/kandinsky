#include <kandinsky/math.h>

#include <kandinsky/print.h>

namespace kdk {

const char* ToString(Arena* arena, const Vec2 v) { return Printf(arena, "(%.3f, %.3f)", v.x, v.y); }

const char* ToString(Arena* arena, const Vec3 v) {
    return Printf(arena, "(%.3f, %.3f, %.3f)", v.x, v.y, v.z);
}

const char* ToString(Arena* arena, const Vec4 v) {
    return Printf(arena, "(%.3f, %.3f, %.3f, %.3f)", v.x, v.y, v.z, v.w);
}

Vec3 TransformPoint(const Mat4& m, const Vec3& point) {
    Vec4 p(point, 1.0f);
    Vec4 transformed = m * p;

    // Convert from homogenous coordinates back to 3D
    return Vec3(transformed.x / transformed.w,
                transformed.y / transformed.w,
                transformed.z / transformed.w);
}

}  // namespace kdk
