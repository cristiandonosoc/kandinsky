#include "parsing.h"

#include <direct.h>  // for getcwd()
#include <array>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <clang-c/Index.h>

std::vector<std::string> readCompileFlags(const std::string& flagsFile) {
    std::vector<std::string> flags;
    std::ifstream file(flagsFile);
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty()) {
            flags.push_back(line);
        }
    }

    return flags;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <file.cpp> <compile_flags.txt>" << std::endl;
        return -1;
    }

    // Check if the source file exists
    std::ifstream file_check(argv[1]);
    if (!file_check.good()) {
        std::cout << "Error: Cannot open source file " << argv[1] << std::endl;
        return -1;
    }

    // Check if the compile flags file exists
    std::ifstream flags_check(argv[2]);
    if (!flags_check.good()) {
        std::cout << "Error: Cannot open compile flags file " << argv[2] << std::endl;
        return -1;
    }

    // Read compile flags from file
    auto args = readCompileFlags(argv[2]);
    for (auto& arg : args) {
        std::cout << "Arg: " << arg << std::endl;
    }
    std::array<const char*, 100> args_array;
    for (size_t i = 0; i < args.size(); ++i) {
        args_array[i] = args[i].c_str();
    }

    std::cout << "Parsing file: " << argv[1] << std::endl;

    // Create index with more diagnostic options
    CXIndex index = clang_createIndex(0, 1);

    CXTranslationUnit tu = nullptr;
    CXErrorCode error = clang_parseTranslationUnit2(index,
                                                    argv[1],
                                                    args_array.data(),
                                                    args.size(),
                                                    nullptr,
                                                    0,
                                                    CXTranslationUnit_DetailedPreprocessingRecord |
                                                        CXTranslationUnit_KeepGoing |
                                                        CXTranslationUnit_SkipFunctionBodies,
                                                    &tu);

    if (error != CXError_Success) {
        std::string errorStr;
        switch (error) {
            case CXError_Failure: errorStr = "CXError_Failure - Generic error"; break;
            case CXError_Crashed: errorStr = "CXError_Crashed - libclang crashed"; break;
            case CXError_InvalidArguments:
                errorStr = "CXError_InvalidArguments - Invalid arguments";
                break;
            case CXError_ASTReadError:
                errorStr = "CXError_ASTReadError - AST deserialization error";
                break;
            default: errorStr = "Unknown error"; break;
        }
        std::cout << "Failed to parse translation unit. Error: " << errorStr << " (code: " << error
                  << ")" << std::endl;
    }

    // Try to get any available diagnostics
    if (tu) {
        unsigned int numDiagnostics = clang_getNumDiagnostics(tu);
        std::cout << "Found " << numDiagnostics << " diagnostics:" << std::endl;

        for (unsigned int i = 0; i < numDiagnostics; i++) {
            CXDiagnostic diagnostic = clang_getDiagnostic(tu, i);
            if (!diagnostic) {
                continue;
            }

            CXString message = clang_getDiagnosticSpelling(diagnostic);
            if (!message.data) {
                clang_disposeDiagnostic(diagnostic);
                continue;
            }

            CXSourceLocation loc = clang_getDiagnosticLocation(diagnostic);
            if (loc.ptr_data[0] == nullptr) {
                std::cout << "Diagnostic: " << clang_getCString(message) << std::endl;
                clang_disposeString(message);
                clang_disposeDiagnostic(diagnostic);
                continue;
            }

            unsigned int line = 0, column = 0;
            CXFile file = nullptr;
            clang_getExpansionLocation(loc, &file, &line, &column, nullptr);

            if (!file) {
                clang_disposeString(message);
                clang_disposeDiagnostic(diagnostic);
                continue;
            }

            CXString fileName = clang_getFileName(file);
            if (!fileName.data) {
                clang_disposeString(message);
                clang_disposeDiagnostic(diagnostic);
                continue;
            }

            const char* fileNameStr = clang_getCString(fileName);
            const char* messageStr = clang_getCString(message);

            if (fileNameStr && messageStr) {
                std::cout << fileNameStr << ":" << line << ":" << column << " - " << messageStr
                          << std::endl;
            }

            clang_disposeString(fileName);
            clang_disposeString(message);
            clang_disposeDiagnostic(diagnostic);
        }
    }

    if (!tu) {
        std::cout << "Failed to create translation unit." << std::endl;
        clang_disposeIndex(index);
        return -1;
    }

    using namespace kdk;

    CXCursor root = clang_getTranslationUnitCursor(tu);

    // std::cout << "Printing all annotations:\n";
    // VisitAllNodes(root);
    // clang_visitChildren(rootCursor, printAllVisitor, &treeLevel);

    std::cout << "Collecting KDK structs:\n";

    Arena arena = AllocateArena("Arena"sv, 100 * MEGABYTE);
    Arena result_arena = AllocateArena("ResultArena"sv, 100 * MEGABYTE);
    DynArray<StructInfo> structs = NewDynArray<StructInfo>(&result_arena);
    CollectKDKStructs(&arena, root, &structs);

    for (i32 i = 0; i < structs.Size; i++) {
        auto& s = structs[i];
        std::cout << "Struct: " << s.Name.Str() << "\n";

        for (i32 ai = 0; ai < s.Fields.Size; ai++) {
            FieldInfo& field = s.Fields[ai];
            std::cout << "  Field: " << field.Name.Str() << ", Type: " << field.TypeName.Str()
                      << ", Canonical Type: " << field.CanonicalTypeName.Str() << "\n";
        }
    }

#if 0

    // Print the collected information
    for (const auto& structInfo : kdkStructs) {
        std::cout << "Struct: " << structInfo.name << "\n";
        std::cout << "  KDK attributes:";
        for (const auto& attr : structInfo.kdkAttributes) {
            std::cout << " \"" << attr << "\"";
        }
        std::cout << "\n";

        std::cout << "  Fields:\n";
        for (const auto& field : structInfo.fields) {
            std::cout << "    " << field.name << " (Type: " << field.type
                      << ", C Type: " << field.canonicalType << ")\n";
        }
        std::cout << "\n";

        // Generate ImGui code for this struct
        std::string imguiCode = generateImGuiCode(structInfo);
        std::cout << "Generated ImGui code for " << structInfo.name << ":\n";
        std::cout << "----------------------------------------\n";
        std::cout << imguiCode;
        std::cout << "----------------------------------------\n\n";

        // // Optionally generate a file
        // std::string outputDir = ".";  // Current directory
        // if (generateImGuiFile(structInfo, outputDir)) {
        //     std::cout << "Successfully wrote ImGui code to imgui_" << structInfo.name << ".h\n\n";
        // } else {
        //     std::cout << "Failed to write ImGui code to file\n\n";
        // }
    }

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

#endif
    return 0;
}
