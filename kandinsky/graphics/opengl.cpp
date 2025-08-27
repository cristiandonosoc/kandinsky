#include <kandinsky/graphics/opengl.h>

#include <kandinsky/core/defines.h>
#include <kandinsky/core/file.h>
#include <kandinsky/core/memory.h>
#include <kandinsky/core/string.h>
#include <kandinsky/core/time.h>
#include <kandinsky/graphics/render_state.h>
#include <kandinsky/graphics/shader.h>
#include <kandinsky/input.h>
#include <kandinsky/platform.h>

#include <SDL3/SDL.h>

#include <stb/stb_image.h>
#include <glm/gtc/matrix_transform.hpp>

namespace kdk {

// Grid --------------------------------------------------------------------------------------------

void DrawGrid(const RenderState& rs, float near, float far) {
    auto* ps = platform::GetPlatformContext();
    if (auto [_, grid_shader] =
            FindAssetT<Shader>(&ps->Assets, ps->Assets.BaseAssets.GridShaderHandle);
        grid_shader) {
        Use(*grid_shader);
        glBindVertexArray(ps->Assets.BaseAssets.GridVAO);
        SetFloat(*grid_shader, "uGridSize", 75);
        SetVec3(*grid_shader, "uCameraPos", rs.CameraPosition);
        SetVec2(*grid_shader, "uFogRange", {near, far});
        SetMat4(*grid_shader, "uM_View", GetPtr(rs.M_View));
        SetMat4(*grid_shader, "uM_Proj", GetPtr(rs.M_Proj));

        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
}

// LineBatcher -------------------------------------------------------------------------------------

void Reset(LineBatcher* lb) {
    lb->Batches.clear();
    lb->Data.clear();
}

void StartLineBatch(LineBatcher* lb, GLenum mode, Color32 color, float line_width) {
    ASSERT(IsValid(*lb));
    ASSERT(lb->CurrentBatch == NONE);
    lb->Batches.push_back({
        .Mode = mode,
        .Color = ToVec4(color),
        .LineWidth = line_width,
    });
    lb->CurrentBatch = static_cast<i32>(lb->Batches.size() - 1);
}

void EndLineBatch(LineBatcher* lb) {
    ASSERT(IsValid(*lb));
    ASSERT(lb->CurrentBatch != NONE);
    lb->CurrentBatch = NONE;
}

void AddPoint(LineBatcher* lb, const Vec3& point) {
    ASSERT(IsValid(*lb));
    ASSERT(lb->CurrentBatch != NONE);

    lb->Batches[lb->CurrentBatch].PrimitiveCount++;

    auto* begin = &point;
    auto* end = begin + 1;
    lb->Data.insert(lb->Data.end(), (u8*)begin, (u8*)end);
}

void AddPoints(LineBatcher* lb, const Vec3& p1, const Vec3& p2) {
    AddPoint(lb, p1);
    AddPoint(lb, p2);
}

void AddPoints(LineBatcher* lb, std::span<const Vec3> points) {
    ASSERT(IsValid(*lb));

    for (const auto& point : points) {
        AddPoint(lb, point);
    }
}

void Buffer(PlatformState*, const LineBatcher& lb) {
    ASSERT(IsValid(lb));

    // Send the data.
    glBindBuffer(GL_ARRAY_BUFFER, lb.VBO);
    glBufferData(GL_ARRAY_BUFFER, lb.Data.size(), lb.Data.data(), GL_STREAM_DRAW);
}

void Draw(const LineBatcher& lb, const Shader& shader) {
    ASSERT(IsValid(lb));

    // Ensure we leave line width as it was.
    float current_line_width = 1.0f;
    glGetFloatv(GL_LINE_WIDTH, &current_line_width);

    glBindVertexArray(lb.VAO);

    GLint primitive_count = 0;
    for (const LineBatch& batch : lb.Batches) {
        SetVec4(shader, "uColor", batch.Color);
        glLineWidth(batch.LineWidth);

        glDrawArrays(batch.Mode, primitive_count, batch.PrimitiveCount);
        primitive_count += batch.PrimitiveCount;
    }

    glLineWidth(current_line_width);
}

LineBatcher* CreateLineBatcher(LineBatcherRegistry* registry, const char* name) {
    ASSERT(registry->LineBatcherCount < LineBatcherRegistry::kMaxLineBatchers);

    GLuint vao = GL_NONE;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo = GL_NONE;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    GLsizei stride = 3 * sizeof(float);
    u64 offset = 0;
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
    glEnableVertexAttribArray(0);

    glBindVertexArray(GL_NONE);

    LineBatcher lb{
        .Name = platform::InternToStringArena(name),
        .ID = IDFromString(name),
        .VAO = vao,
        .VBO = vbo,
    };
    lb.Data.reserve(128);

    registry->LineBatchers[registry->LineBatcherCount++] = std::move(lb);
    return &registry->LineBatchers[registry->LineBatcherCount - 1];
}

LineBatcher* FindLineBatcher(LineBatcherRegistry* registry, i32 id) {
    for (i32 i = 0; i < registry->LineBatcherCount; i++) {
        auto& lb = registry->LineBatchers[i];
        if (lb.ID == id) {
            return &lb;
        }
    }

    return nullptr;
}

// Material ----------------------------------------------------------------------------------------

MaterialAssetHandle CreateMaterial(AssetRegistry* assets,
                                   String asset_path,
                                   const CreateMaterialOptions& options) {
    if (MaterialAssetHandle found = FindMaterialHandle(assets, asset_path); IsValid(found)) {
        return found;
    }

    ASSERT(!assets->MaterialHolder.IsFull());

    i32 asset_id = GenerateAssetID(EAssetType::Texture, asset_path);
    Material material{
        .ID = asset_id,
        .Albedo = options.Albedo,
        .Diffuse = options.Diffuse,
        .Shininess = options.Shininess,
    };
    material.TextureHandles.Push(options.TextureHandles);

    SDL_Log("Created material %s\n", asset_path.Str());
    AssetHandle result =
        assets->MaterialHolder.PushAsset(asset_id, asset_path, std::move(material));
    return {result};
}

}  // namespace kdk
