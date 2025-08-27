#pragma once

#include <kandinsky/asset.h>
#include <kandinsky/core/math.h>
#include <kandinsky/core/string.h>

#include <GL/glew.h>

namespace kdk {

struct Shader {
	GENERATE_ASSET(Shader);

    i32 ID = NONE;
    String Path = {};

    SDL_Time LastLoadTime = 0;

    GLuint Program = GL_NONE;
};

inline bool IsValid(const Shader& shader) { return shader.Program != GL_NONE; }

void Use(const Shader& shader);
void SetBool(const Shader& shader, const char* uniform, bool value);
void SetI32(const Shader& shader, const char* uniform, i32 value);
void SetU32(const Shader& shader, const char* uniform, u32 value);
void SetFloat(const Shader& shader, const char* uniform, float value);
void SetUVec2(const Shader& shader, const char* uniform, const UVec2& value);
void SetUVec3(const Shader& shader, const char* uniform, const UVec3& value);
void SetUVec4(const Shader& shader, const char* uniform, const UVec4& value);
void SetVec2(const Shader& shader, const char* uniform, const Vec2& value);
void SetVec3(const Shader& shader, const char* uniform, const Vec3& value);
void SetVec4(const Shader& shader, const char* uniform, const Vec4& value);
void SetMat4(const Shader& shader, const char* uniform, const float* value);

ShaderAssetHandle CreateShader(AssetRegistry* assets, String asset_path);
bool ReevaluateShaders(AssetRegistry* assets);

}  // namespace kdk
