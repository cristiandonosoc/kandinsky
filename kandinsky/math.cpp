#include <SDL3/SDL_log.h>
#include <kandinsky/math.h>

#include <kandinsky/string.h>

namespace kdk {

String ToString(Arena* arena, const Vec2& v) { return Printf(arena, "(%.3f, %.3f)", v.x, v.y); }

String ToString(Arena* arena, const Vec3& v) {
    return Printf(arena, "(%.3f, %.3f, %.3f)", v.x, v.y, v.z);
}

String ToString(Arena* arena, const Vec4& v) {
    return Printf(arena, "(%.3f, %.3f, %.3f, %.3f)", v.x, v.y, v.z, v.w);
}

Plane ComputePlane(const Vec3& p1, const Vec3& p2, const Vec3& p3) {
    Plane p;

    p.Normal = Normalize(Cross(p2 - p1, p3 - p1));
    p.Distance = Dot(p1, p.Normal);

    return p;
}

bool IntersectPlaneRay(const Plane& plane,
                       const Vec3& ray_origin,
                       const Vec3& ray_dir,
                       Vec3* out_intersection) {
    float t = (plane.Distance - Dot(ray_origin, plane.Normal)) / Dot(ray_dir, plane.Normal);
    if (t >= 0.0f) {
        *out_intersection = ray_origin + t * ray_dir;
        return true;
    }

    // No intersection.
    return false;
}

Vec3 TransformPoint(const Mat4& m, const Vec3& point) {
    Vec4 p(point, 1.0f);
    Vec4 transformed = m * p;

    // Convert from homogenous coordinates back to 3D
    return Vec3(transformed.x / transformed.w,
                transformed.y / transformed.w,
                transformed.z / transformed.w);
}

void CalculateModelMatrix(const Transform& transform, Mat4* out_model) {
    Mat4 mmodel(1.0f);
    mmodel = Translate(mmodel, transform.Position);
    mmodel = Rotate(mmodel, transform.Rotation);
    mmodel = Scale(mmodel, Vec3(transform.Scale));

    *out_model = mmodel;
}

}  // namespace kdk
