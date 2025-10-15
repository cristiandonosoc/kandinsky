#pragma once

#include <kandinsky/core/defines.h>
#include <kandinsky/core/string.h>

namespace kdk {

enum class EArgType : u8 {
    Invalid,
    Boolean,
    String,
    Int,
    Float,
};
const char* ToString(EArgType type);

struct ArgParser {
    static constexpr u32 kMaxArguments = 32;

    struct Argument {
        EArgType Type = EArgType::Invalid;
        String LongName = {};
        char ShortName = NULL;

        bool Required = false;

        bool HasValue = 0;
        union {
            bool BoolValue;
            String StringValue;
            i32 IntValue;
            float FloatValue;
        };
    };

    const char* AppName = nullptr;
    Argument Arguments[kMaxArguments] = {};
    u32 ArgCount = 0;
};

void AddStringArgument(ArgParser* ap, String long_name, char short_name, bool required);
void AddIntArgument(ArgParser* ap, String long_name, char short_name, bool required);
void AddFloatArgument(ArgParser* ap, String long_name, char short_name, bool required);

bool ParseArguments(ArgParser* ap, int argc, const char* argv[]);

bool FindBoolValue(const ArgParser& ap, String long_name, bool* out);
bool FindStringValue(const ArgParser& ap, String long_name, String* out);
bool FindIntValue(const ArgParser& ap, String long_name, i32* out);
bool FindFloatValue(const ArgParser& ap, String long_name, float* out);

}  // namespace kdk
