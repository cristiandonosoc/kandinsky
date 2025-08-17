#pragma once

#include <kandinsky/core/string.h>

#include <source_location>
#include <span>

namespace kdk {

struct Arena;

bool SaveFile(String path, std::span<u8> data);

struct LoadFileOptions {
	bool NullTerminate : 1 = false;
};
std::span<u8> LoadFile(Arena* arena, String path, const LoadFileOptions& options = {});

void ListEnv();

namespace testing {

bool RunningUnderBazel();

// Determines where the test data directory is.
// This will do the transformation from bazel (if running under it).
std::span<String> ListTestDataFiles(Arena* arena, String subpath = {});

}  // namespace testing

}  // namespace kdk
