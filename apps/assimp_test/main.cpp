#include <kandinsky/defines.h>

#include <kandinsky/opengl.h>

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include <SDL3/SDL.h>

int main() {
    Assimp::Importer importer;

	const char* path = "D:/assets/Survival_BackPack_2/Survival_BackPack_2.fbx";
	SDL_PathInfo info = {};
	if (!SDL_GetPathInfo(path, &info)) {
		SDL_Log("ERROR: GetPathInfo %s: %s\n", path, SDL_GetError());
		return -1;
	}


    const aiScene* scene =
        importer.ReadFile(path,
                          aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals |
                              aiProcess_CalcTangentSpace);
	if (!scene) {
		SDL_Log("ERROR: Loading model at %s: %s\n", path, importer.GetErrorString());
		return -1;
	}

	SDL_Log("Model at %s loaded!\n", path);

	SDL_Log("Meshes: %d\n", scene->mNumMeshes);
	for (u32 i = 0; i < scene->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		SDL_Log("- %s\n", mesh->mName.C_Str());
	}
}
