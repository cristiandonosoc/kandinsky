#include <learn_opengl/light.h>

#include <kandinsky/math.h>
#include <kandinsky/opengl.h>

namespace kdk {

const char* ToString(ELightType v) {
    switch (v) {
        case ELightType::Point:
            return "Point";
        case ELightType::Directional:
            return "Directional";
        case ELightType::Spotlight:
            return "Spotlight";
        case ELightType::COUNT:
            return "<COUNT>";
            break;
    }

    assert(false);
    return "<UNKNOWN>";
}

void RenderLight(PlatformState* ps, RenderState_Light* rs) {
    if (rs->Light->Type == ELightType::Directional) {
        return;
    }

    Use(ps, *rs->Shader);
    Bind(ps, *rs->Mesh);

    SetMat4(ps, *rs->Shader, "uViewProj", glm::value_ptr(*rs->ViewProj));

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(rs->Light->Position));
    model = glm::scale(model, glm::vec3(0.2f));
    SetMat4(ps, *rs->Shader, "uModel", glm::value_ptr(model));
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

}  // namespace kdk
