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

}  // namespace kdk
