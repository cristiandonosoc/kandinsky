#include <catch2/catch_test_macros.hpp>

#include <kandinsky/core/file.h>
#include <kandinsky/core/memory.h>

#include <SDL3/SDL_iostream.h>

#include <iostream>

using namespace kdk;

TEST_CASE("Load file", "[file]") {
    // TODO(cdc): We need a way to find the test files from normal running.
    if (!testing::RunningUnderBazel()) {
        return;
    }

    auto scratch = GetScratchArena();

    auto files = testing::ListTestDataFiles(scratch.Arena, String("file_test/simple_file.txt"));
    std::cout << files.size() << " files found." << std::endl;
    REQUIRE(!files.empty());
    for (const auto& file : files) {
        auto data = LoadFile(scratch.Arena, file);
        REQUIRE(!data.empty());
        REQUIRE(data.size_bytes() == 12);
    }
}
