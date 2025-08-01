#include <kandinsky/graphics/model.h>

#include <kandinsky/platform.h>

namespace kdk {

void OnLoadedOnEntity(Entity* entity, StaticModelComponent* component) {
    ASSERT(entity);
    ASSERT(component);
}

void LoadAssets(StaticModelComponent* component) {
    auto scratch = GetScratchArena();

    if (!component->ModelPath.IsEmpty()) {
        ModelRegistry* registry = &platform::GetPlatformContext()->Models;
        component->Model = CreateModel(scratch.Arena, registry, component->ModelPath);
        if (!component->Model) {
            SDL_Log("ERROR: Failed to load model %s\n", component->ModelPath.Str());
        }
    }

    if (!component->ShaderPath.IsEmpty()) {
        ShaderRegistry* registry = &platform::GetPlatformContext()->Shaders.Registry;
        component->Shader = CreateShader(registry, component->ShaderPath);
        if (!component->Shader) {
            SDL_Log("ERROR: Failed to load shader %s\n", component->ShaderPath.Str());
        }
    }
}

}  // namespace kdk
