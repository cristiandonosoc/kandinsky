#pragma once

#include <kandinsky/entity.h>
#include <kandinsky/graphics/opengl.h>

namespace kdk {

struct StaticModelComponent {
    GENERATE_COMPONENT(StaticModel);

    String ModelPath;
    String ShaderPath;

    Shader* Shader = nullptr;
    Model* Model = nullptr;
};

void OnLoadedOnEntity(Entity* entity, StaticModelComponent* component);
void LoadAssets(StaticModelComponent* component);

}  // namespace kdk
