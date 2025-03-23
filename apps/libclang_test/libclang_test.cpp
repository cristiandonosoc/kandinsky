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

CXChildVisitResult printAllVisitor(CXCursor cursor, CXCursor parent, CXClientData clientData) {
    CXSourceLocation location = clang_getCursorLocation(cursor);
    if (clang_Location_isFromMainFile(location) == 0) {
        return CXChildVisit_Continue;
    }

    unsigned int curLevel = *(reinterpret_cast<unsigned int*>(clientData));
    unsigned int nextLevel = curLevel + 1;
    CXCursorKind cursorKind = clang_getCursorKind(cursor);

    // Get location information
    unsigned int line, column;
    CXFile file;
    clang_getExpansionLocation(location, &file, &line, &column, nullptr);

    // Print cursor information
    std::cout << std::string(curLevel, '-') << " " << getCursorKindName(cursorKind) << " '"
              << getCursorSpelling(cursor) << "' "
              << "at line " << line << ":" << column;
    std::cout << "\n";

    // Visit all other children
    clang_visitChildren(cursor, printAllVisitor, &nextLevel);
    return CXChildVisit_Continue;
}

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
    CXCursor rootCursor = clang_getTranslationUnitCursor(tu);

    unsigned int treeLevel = 0;

    std::cout << "Printing all annotations:\n";
    clang_visitChildren(rootCursor, printAllVisitor, &treeLevel);

    std::cout << "Collecting KDK structs:\n";
    auto kdkStructs = collectKDKStructs(rootCursor);

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
    }

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    return 0;
}
