#include <catch2/catch_test_macros.hpp>

#include <kandinsky/core/container.h>
#include <kandinsky/utils/arg_parser.h>

using namespace kdk;

TEST_CASE("ArgParser", "[arg_parser]") {
    SECTION("String") {
        ArgParser ap = {};
        AddStringArgument(&ap, "long_flag", NULL, false);
        AddStringArgument(&ap, "another_long_flag", 'a', false);
        AddStringArgument(&ap, "yet_another", 'y', false);

        // clang-format off
		Array argv {
			"kandinsky",
			"--long_flag", "long_flag value",
			"--another_long_flag", "another_long_flag value",
			"-y", "yet_another value",
		};
        // clang-format on

        bool ok = ParseArguments(&ap, argv.Size, argv.DataPtr());
        REQUIRE(ok);

        {
            const char* value = nullptr;
            REQUIRE(FindStringValue(ap, "long_flag", &value));
            REQUIRE(strcmp(value, "long_flag value") == 0);
        }

        {
            const char* value = nullptr;
            REQUIRE(FindStringValue(ap, "another_long_flag", &value));
            REQUIRE(strcmp(value, "another_long_flag value") == 0);
        }

        {
            const char* value = nullptr;
            REQUIRE(FindStringValue(ap, "yet_another", &value));
            REQUIRE(strcmp(value, "yet_another value") == 0);
        }
    }

    SECTION("Required") {
        ArgParser ap = {};
        AddStringArgument(&ap, "long_flag", NULL, false);
        AddStringArgument(&ap, "another_long_flag", 'a', false);
        AddStringArgument(&ap, "yet_another", 'y', false);
        AddStringArgument(&ap, "required", NULL, true);

        // clang-format off
		Array argv {
			"kandinsky",
			"--long_flag", "long_flag value",
			"--another_long_flag", "another_long_flag value",
			"-y", "yet_another value",
		};
        // clang-format on

        bool ok = ParseArguments(&ap, argv.Size, argv.DataPtr());
        REQUIRE(!ok);
    }
}
