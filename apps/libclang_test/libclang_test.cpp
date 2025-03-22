#include <clang-c/Index.h>

#include <direct.h>  // for getcwd()
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

std::string getCursorKindName(CXCursorKind cursorKind) {
    CXString kindName = clang_getCursorKindSpelling(cursorKind);
    std::string result = clang_getCString(kindName);

    clang_disposeString(kindName);
    return result;
}

std::string getCursorSpelling(CXCursor cursor) {
    CXString cursorSpelling = clang_getCursorSpelling(cursor);
    std::string result = clang_getCString(cursorSpelling);

    clang_disposeString(cursorSpelling);
    return result;
}

// Replace hasKDKAnnotation with this new function that collects the arguments
std::vector<std::string> getKDKAnnotationArgs(CXCursor cursor) {
    std::vector<std::string> args;
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor /* parent */, CXClientData clientData) {
            if (clang_getCursorKind(c) != CXCursor_AnnotateAttr) {
                return CXChildVisit_Continue;
            }

            // Check if this is a KDK annotation
            CXString spelling = clang_getCursorSpelling(c);
            const char* text = clang_getCString(spelling);
            if (strcmp(text, "KDK") != 0) {
                clang_disposeString(spelling);
                return CXChildVisit_Continue;
            }

            auto args_ptr = reinterpret_cast<std::vector<std::string>*>(clientData);

            // Visit children of the annotation to get the UnexposedExpr arguments
            clang_visitChildren(
                c,
                [](CXCursor expr, CXCursor /* parent */, CXClientData innerData) {
                    if (clang_getCursorKind(expr) != CXCursor_UnexposedExpr) {
                        return CXChildVisit_Continue;
                    }

                    // Get the StringLiteral child of the UnexposedExpr
                    clang_visitChildren(
                        expr,
                        [](CXCursor strLit, CXCursor /* parent */, CXClientData strData) {
                            if (clang_getCursorKind(strLit) != CXCursor_StringLiteral) {
                                return CXChildVisit_Continue;
                            }

                            CXString argSpelling = clang_getCursorSpelling(strLit);
                            std::string arg = clang_getCString(argSpelling);
                            // Remove quotes from beginning and end if present
                            if (arg.size() >= 2 && arg.front() == '"' && arg.back() == '"') {
                                arg = arg.substr(1, arg.size() - 2);
                            }
                            reinterpret_cast<std::vector<std::string>*>(strData)->push_back(arg);
                            clang_disposeString(argSpelling);
                            return CXChildVisit_Continue;
                        },
                        innerData);
                    return CXChildVisit_Continue;
                },
                args_ptr);

            clang_disposeString(spelling);
            return CXChildVisit_Continue;
        },
        &args);
    return args;
}

CXChildVisitResult memberVisitor(CXCursor cursor, CXCursor /* parent */, CXClientData clientData) {
    CXSourceLocation location = clang_getCursorLocation(cursor);
    if (clang_Location_isFromMainFile(location) == 0) {
        return CXChildVisit_Continue;
    }

    unsigned int curLevel = *(reinterpret_cast<unsigned int*>(clientData));
    CXCursorKind cursorKind = clang_getCursorKind(cursor);

    // Print member info
    if (cursorKind == CXCursor_FieldDecl) {
        std::cout << std::string(curLevel, '-') << " Field: " << getCursorSpelling(cursor) << "\n";
    }

    return CXChildVisit_Continue;
}

// Then modify the visitor to use the new function
CXChildVisitResult visitor(CXCursor cursor, CXCursor /* parent */, CXClientData clientData) {
    CXSourceLocation location = clang_getCursorLocation(cursor);
    if (clang_Location_isFromMainFile(location) == 0) {
        return CXChildVisit_Continue;
    }

    CXCursorKind cursorKind = clang_getCursorKind(cursor);
    if (cursorKind != CXCursor_StructDecl && cursorKind != CXCursor_ClassDecl) {
        unsigned int curLevel = *(reinterpret_cast<unsigned int*>(clientData));
        unsigned int nextLevel = curLevel + 1;

        // Visit children of the struct to show its members
        clang_visitChildren(cursor, visitor, &nextLevel);
        return CXChildVisit_Continue;
    }

    // Only process structs/classes
    std::cout << "Processing struct/class: " << getCursorSpelling(cursor) << std::endl;
    std::vector<std::string> kdkArgs = getKDKAnnotationArgs(cursor);
    if (!kdkArgs.empty()) {
        unsigned int curLevel = *(reinterpret_cast<unsigned int*>(clientData));
        unsigned int nextLevel = curLevel + 1;

        std::cout << std::string(curLevel, '-') << " " << getCursorKindName(cursorKind) << " ("
                  << getCursorSpelling(cursor) << ")\n";

        // Print KDK arguments
        std::cout << std::string(nextLevel, '-') << " KDK args:";
        for (const auto& arg : kdkArgs) {
            std::cout << " \"" << arg << "\"";
        }
        std::cout << "\n";

        // Visit children of the struct to show its members
        clang_visitChildren(cursor, memberVisitor, &nextLevel);
    }

    return CXChildVisit_Continue;
}

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

    // // Print parent information if available
    // if (!clang_Cursor_isNull(parent)) {
    //     std::cout << " (parent: " << getCursorKindName(clang_getCursorKind(parent))
    //               << " '" << getCursorSpelling(parent) << "')";
    // }
    std::cout << "\n";

    // // Print any annotations and their children
    // clang_visitChildren(
    //     cursor,
    //     [](CXCursor c, CXCursor parent, CXClientData clientData) {
    //         unsigned int level = *(reinterpret_cast<unsigned int*>(clientData));
    //         unsigned int nextLevel = level + 1;

    //         if (clang_getCursorKind(c) == CXCursor_AnnotateAttr) {
    //             CXString spelling = clang_getCursorSpelling(c);
    //             std::cout << std::string(level + 1, '-') << " Annotation: "
    //                      << clang_getCString(spelling) << "\n";
    //             clang_disposeString(spelling);

    //             // Visit children of the annotation
    //             clang_visitChildren(c, printAllVisitor, &nextLevel);
    //         }
    //         return CXChildVisit_Continue;
    //     },
    //     &nextLevel
    // );

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
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <file.cpp>" << std::endl;
        return -1;
    }

    // Check if the file exists first (move this before parsing)
    std::ifstream file_check(argv[1]);
    if (!file_check.good()) {
        std::cout << "Error: Cannot open file " << argv[1] << std::endl;
        return -1;
    }

    // Add this before parsing to verify paths
    std::cout << "Checking include paths:" << std::endl;
    const char* paths_to_check[] = {"third_party",
                                    "third_party/LLVM-20.1.0/include",
                                    "extras/includes"};
    for (const char* path : paths_to_check) {
        std::ifstream check(path);
        std::cout << "Path '" << path << "' " << (check.good() ? "exists" : "does not exist")
                  << std::endl;
    }

    const char* args[] = {
        "-xc++",
        "--std=c++20",
        "-E",   // Run preprocessor
        "-P",   // Don't generate linemarkers
        "-dD",  // Keep macro definitions
        "-CC",  // Keep comments
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-Werror",
        "-Wimplicit-fallthrough",
        "-Wno-gnu-anonymous-struct",
        "-Wno-nested-anon-types",
        "-Wno-unused-const-variable",
        "-Wno-extra-semi",
        "-Wno-pragma-once-outside-header",
        "-Wno-unknown-attributes",
        "-I.",
        "-Iapps",
        "-isystem",
        "third_party",
        "-isystem",
        "third_party/SDL3-3.2.2/include",
        "-isystem",
        "third_party/glew-2.2.0/include",
        "-isystem",
        "third_party/imgui-1.91.8",
        "-isystem",
        "third_party/imgui-1.91.8/backends",
        "-isystem",
        "third_party/ImGuizmo-master",
        "-isystem",
        "third_party/Catch2-3.8.0/src",
        "-isystem",
        "third_party/assimp-5.4.3/include",
        "-isystem",
        "third_party/cwalk-1.2.9/include",
        "-isystem",
        "third_party/LLVM-20.1.0/include",
        "-isystem",
        "extras/includes",
        "-D",
        "KDK_ATTRIBUTE_GENERATION",
    };

    // Add absolute path printing for debugging
    std::cout << "Current working directory: ";
    char cwd[1024];
    if (_getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << cwd << std::endl;
    }

    // Print the arguments being used
    std::cout << "Using compilation arguments:" << std::endl;
    for (size_t i = 0; i < sizeof(args) / sizeof(args[0]); ++i) {
        std::cout << "  " << args[i] << std::endl;
    }
    std::cout << "Parsing file: " << argv[1] << std::endl;

    CXErrorCode error;
    CXTranslationUnit tu = nullptr;

    // Create index with more diagnostic options
    CXIndex index = clang_createIndex(0, 1);

    error = clang_parseTranslationUnit2(index,
                                        argv[1],
                                        args,
                                        sizeof(args) / sizeof(args[0]),
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

    std::cout << "Printing KDK structs:\n";
    clang_visitChildren(rootCursor, visitor, &treeLevel);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    return 0;
}
