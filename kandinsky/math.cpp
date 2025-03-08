#include <kandinsky/math.h>

#include <kandinsky/print.h>

namespace kdk {

void Transform::SetPosition(const Vec3& position) {
    _Position = position;
    Flags.Dirty = true;
}

void Transform::SetRotation(const Quat& rotation) {
    _Rotation = rotation;
    Flags.Dirty = true;
}

void Transform::AddRotation(const Vec3& axis, float deg) {
    Quat rotation = glm::angleAxis(glm::radians(deg), glm::normalize(axis));
    _Rotation = rotation * _Rotation;
    Flags.Dirty = true;
}

void Transform::SetScale(float scale) {
    _Scale = scale;
    Flags.Dirty = true;
}

const char* ToString(Arena* arena, const Vec2 v) { return Printf(arena, "(%.3f, %.3f)", v.x, v.y); }

const char* ToString(Arena* arena, const Vec3 v) {
    return Printf(arena, "(%.3f, %.3f, %.3f)", v.x, v.y, v.z);
}

const char* ToString(Arena* arena, const Vec4 v) {
    return Printf(arena, "(%.3f, %.3f, %.3f, %.3f)", v.x, v.y, v.z, v.w);
}

}  // namespace kdk
