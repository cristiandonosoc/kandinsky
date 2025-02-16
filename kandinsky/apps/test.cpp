#include <kandinsky/game.h>

#include <kandinsky/imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

extern "C" {

namespace kdk {

// clang-format off
float kVertices[] = {
    // positions          // normals           // texture coords
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
};

glm::vec3 kCubePositions[] = {glm::vec3(0.0f, 0.0f, 0.0f),
                              glm::vec3(2.0f, 5.0f, -15.0f),
                              glm::vec3(-1.5f, -2.2f, -2.5f),
                              glm::vec3(-3.8f, -2.0f, -12.3f),
                              glm::vec3(2.4f, -0.4f, -3.5f),
                              glm::vec3(-1.7f, 3.0f, -7.5f),
                              glm::vec3(1.3f, -2.0f, -2.5f),
                              glm::vec3(1.5f, 2.0f, -2.5f),
                              glm::vec3(1.5f, 0.2f, -1.5f),
                              glm::vec3(-1.3f, 1.0f, -1.5f)};
// clang-format on

bool GameInit(PlatformState* ps) {
    // Meshes.
    if (!CreateMesh(ps,
                    &ps->Meshes,
                    "CubeMesh",
                    {
                        .Vertices = {kVertices, std::size(kVertices)},
                        .AttribPointers = {3, 3, 2},
                    })) {
        return false;
    }

    {
        std::string vs_path = ps->BasePath + "assets/shaders/shader.vert";
        std::string fs_path = ps->BasePath + "assets/shaders/shader.frag";
        if (!CreateShader(ps, &ps->Shaders, "NormalShader", vs_path.c_str(), fs_path.c_str())) {
            return false;
        }
    }

    return true;
}

bool GameUpdate(PlatformState*) { return true; }

bool GameRender(PlatformState* ps) {
    ImGui::ShowDemoWindow(&ps->ShowDebugWindow);

    Mesh* cube_mesh = FindMesh(&ps->Meshes, "CubeMesh");
    assert(cube_mesh);

    Shader* normal_shader = FindShader(&ps->Shaders, "NormalShader");
    assert(normal_shader);

    float seconds = 0;

    ps->glClearColor(0.5f, 0.0f, 0.5f, 1.0f);
    ps->glClear(GL_COLOR_BUFFER_BIT);

    glm::mat4 view = GetViewMatrix(ps->FreeCamera);
    float aspect_ratio = (float)(ps->Window.Width) / (float)(ps->Window.Height);

    /* constexpr float kLightRadius = 3.0f; */
    /* float light_rot_speed = 2 * seconds; */
    /* gLightPosition = */
    /*     glm::vec3(kLightRadius * cos(light_rot_speed), 1.0f, kLightRadius *
     * sin(light_rot_speed)); */

    glm::mat4 proj = glm::mat4(1.0f);
    proj = glm::perspective(glm::radians(45.0f), aspect_ratio, 0.1f, 100.0f);
    /* glm::mat4 view_proj = proj * view; */

    // Render cubes.
    {
        Use(ps, *normal_shader);
        Bind(ps, *cube_mesh);

        // Set the material indices.
        /* SetI32(ps, *normal_shader, "uMaterial.Diffuse", 0); */
        /* SetI32(ps, *normal_shader, "uMaterial.Specular", 1); */
        /* SetI32(ps, *normal_shader, "uMaterial.Emission", 2); */
        /* Bind(ps, *diffuse_texture, GL_TEXTURE0); */
        /* Bind(ps, *specular_texture, GL_TEXTURE1); */
        /* Bind(ps, *emission_texture, GL_TEXTURE2); */

        SetVec3(ps, *normal_shader, "uMaterial.Specular", glm::vec3(0.5f, 0.5f, 0.5f));
        SetFloat(ps, *normal_shader, "uMaterial.Shininess", 32.0f);

        const auto& light_position = ps->LightPosition;
        glm::vec4 view_light_position = view * glm::vec4(light_position, 1.0f);
        SetVec4(ps, *normal_shader, "uLight.PosDir", view_light_position);
        SetVec3(ps, *normal_shader, "uLight.Ambient", glm::vec3(0.2f, 0.2f, 0.2f));
        SetVec3(ps, *normal_shader, "uLight.Diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
        SetVec3(ps, *normal_shader, "uLight.Specular", glm::vec3(1.0f, 1.0f, 1.0f));
        SetFloat(ps, *normal_shader, "uLight.Attenuation.Constant", 1.0f);
        SetFloat(ps, *normal_shader, "uLight.Attenuation.Linear", 5 * 0.09f);
        SetFloat(ps, *normal_shader, "uLight.Attenuation.Quadratic", 5 * 0.032f);

        glm::vec4 spotlight_target = view * glm::vec4(0);
        glm::vec3 spotlight_direction = spotlight_target - view_light_position;
        SetVec3(ps, *normal_shader, "uLight.Spotlight.Direction", spotlight_direction);
        SetFloat(ps, *normal_shader, "uLight.Spotlight.Cutoff", glm::cos(glm::radians(12.5f)));

        SetFloat(ps, *normal_shader, "uTime", seconds);

        SetMat4(ps, *normal_shader, "uProj", glm::value_ptr(proj));

        for (const auto& position : kCubePositions) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, position);
            model = glm::rotate(model, glm::radians(seconds * 25), glm::vec3(1.0f, 0.0f, 0.0f));

            glm::mat4 view_model = view * model;
            glm::mat4 normal_matrix = glm::transpose(glm::inverse(view_model));

            SetMat4(ps, *normal_shader, "uViewModel", glm::value_ptr(view_model));
            SetMat4(ps, *normal_shader, "uNormalMatrix", glm::value_ptr(normal_matrix));

            // glDrawArrays(GL_TRIANGLES, 0, 3);
            /* glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); */
            ps->glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    return true;
}

}  // namespace kdk
}  // namespace kdk
