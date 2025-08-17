#include <kandinsky/core/file.h>

#include <kandinsky/core/container.h>
#include <kandinsky/core/memory.h>

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

#include <iostream>
#include <ranges>
#include <string>

#include <windows.h>

#include <Processenv.h>

namespace kdk {

bool SaveFile(String path, std::span<u8> data) {
    bool ok = SDL_SaveFile(path.Str(), data.data(), data.size_bytes());
    if (!ok) {
        SDL_Log("ERROR: Saving file %s: %s", path.Str(), SDL_GetError());
        return false;
    }
    return true;
}

std::span<u8> LoadFile(Arena* arena, String path, const LoadFileOptions& options) {
    u64 size = 0;
    void* data = SDL_LoadFile(path.Str(), &size);
    if (!data) {
        SDL_Log("ERROR: Loading file %s: %s", path.Str(), SDL_GetError());
        return {};
    }
    DEFER { SDL_free(data); };

    // SDL_LoadFile does not null-terminate the data, so we add an extra byte.
    if (options.NullTerminate) {
        size += 1;
    }
    auto result = ArenaCopy(arena, {(u8*)data, size});
    if (result.empty()) {
        SDL_Log("ERROR: Copying file %s to arena", path.Str());
        return {};
    }
    if (options.NullTerminate) {
        result[size] = 0;  // Null-terminate the data.
    }

    return result;
}

void ListEnv() {
    // Get the environment block (ANSI version)
    LPCH envStrings = GetEnvironmentStrings();
    if (envStrings == nullptr) {
        std::cerr << "Failed to get environment strings" << std::endl;
        return;
    }

    // Parse each environment variable
    LPCH current = envStrings;
    while (*current != '\0') {
        std::string envVar(current);
        size_t pos = envVar.find('=');

        if (pos != std::string::npos) {
            std::string key = envVar.substr(0, pos);
            std::string value = envVar.substr(pos + 1);
            std::cout << key << " = " << value << std::endl;
        } else {
            // Environment variable without value (rare)
            std::cout << envVar << " = (no value)" << std::endl;
        }

        // Move to next environment variable
        current += strlen(current) + 1;
    }

    // Clean up
    FreeEnvironmentStrings(envStrings);
}

namespace testing {

bool RunningUnderBazel() {
    auto scratch = GetScratchArena();
    String env = GetEnv(scratch.Arena, "BAZEL_TEST");
    return !env.IsEmpty();
}

std::span<String> ListTestDataFiles(Arena* arena, String subpath) {
    using namespace paths;

    String manifest_path = GetEnv(arena, "RUNFILES_MANIFEST_FILE");
    if (manifest_path.IsEmpty()) {
        return {};
    }

    auto data = LoadFile(arena, manifest_path);
    if (data.empty()) {
        SDL_Log("ERROR: loading manifest file at %s", manifest_path.Str());
        return {};
    }

    std::string input((const char*)data.data(), data.size_bytes());

    auto lines =
        input | std::views::split('\n') | std::views::transform([](auto&& range) {
            return std::string_view(range.begin(), range.end());
        }) |
        std::views::filter([](std::string_view line) {
            return !line.empty() && line.find_first_not_of(" \t\r\n") != std::string_view::npos;
        });

    auto* array = ArenaPushZero<FixedArray<String, 128>>(arena);

    for (auto line : lines) {
        if (auto spacePos = line.find(' '); spacePos != std::string_view::npos) {
            auto key = line.substr(0, spacePos);
            auto value = line.substr(spacePos + 1);

            if (key.find("testing/testdata") == std::string_view::npos) {
                continue;
            }

            if (!subpath.IsEmpty()) {
                if (key.find(subpath.Str()) == std::string_view::npos) {
                    continue;
                }
            }

            std::cout << "FOUND: " << value << std::endl;
            array->Push(InternStringToArena(arena, String(value)));
        }
    }

    return array->ToSpan();
}

}  // namespace testing

}  // namespace kdk
