#include "parsing.h"

#include <kandinsky/core/string.h>

#include <clang-c/Index.h>

#include <fstream>
#include <functional>
#include <iostream>

namespace kdk {

namespace parsing_private {

const char* GetCursorKindName(Arena* arena, CXCursorKind cursorKind) {
    CXString kindName = clang_getCursorKindSpelling(cursorKind);
    const char* result = InternStringToArena(arena, clang_getCString(kindName));
    clang_disposeString(kindName);
    return result;
}

const char* GetCursorSpelling(Arena* arena, CXCursor cursor) {
    CXString cursorSpelling = clang_getCursorSpelling(cursor);
    const char* result = InternStringToArena(arena, clang_getCString(cursorSpelling));
    clang_disposeString(cursorSpelling);
    return result;
}

const char* InternCXString(Arena* arena, CXString string) {
    const char* result = InternStringToArena(arena, clang_getCString(string));
    clang_disposeString(string);
    return result;
}

template <typename T>
void WrappedVisitChildren(CXCursor cursor, T visitor) {
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor p, CXClientData d) -> CXChildVisitResult {
            return (*static_cast<T*>(d))(c, p, nullptr);
        },
        &visitor);
}

void GetKDKAnnotationArgs(Arena* arena, const CXCursor& cursor, FixedArray<const char*, 8>* out) {
    auto visitor = [arena, out](CXCursor c,
                                CXCursor /* parent */,
                                CXClientData /* clientData */) -> CXChildVisitResult {
        if (clang_getCursorKind(c) != CXCursor_AnnotateAttr) {
            return CXChildVisit_Continue;
        }

        // Check if this is a KDK annotation
        CXString spelling = clang_getCursorSpelling(c);
        const char* text = clang_getCString(spelling);
        DEFER { clang_disposeString(spelling); };
        if (strcmp(text, "KDK") != 0) {
            return CXChildVisit_Continue;
        }

        // Visit children of the annotation to get the UnexposedExpr arguments
        WrappedVisitChildren(c, [arena, out](CXCursor expr, CXCursor, CXClientData) {
            if (clang_getCursorKind(expr) != CXCursor_UnexposedExpr) {
                return CXChildVisit_Continue;
            }

            // Get the StringLiteral child of the UnexposedExpr
            WrappedVisitChildren(expr, [arena, out](CXCursor strLit, CXCursor, CXClientData) {
                if (clang_getCursorKind(strLit) != CXCursor_StringLiteral) {
                    return CXChildVisit_Continue;
                }

                CXString argSpelling = clang_getCursorSpelling(strLit);
                std::string arg = clang_getCString(argSpelling);
                DEFER { clang_disposeString(argSpelling); };

                // Remove quotes from beginning and end if present
                if (arg.size() >= 2 && arg.front() == '"' && arg.back() == '"') {
                    arg = arg.substr(1, arg.size() - 2);
                }

                out->Push(InternStringToArena(arena, arg.c_str()));

                return CXChildVisit_Continue;
            });
            return CXChildVisit_Continue;
        });

        return CXChildVisit_Continue;
    };

    WrappedVisitChildren(cursor, visitor);
}

}  // namespace parsing_private

void VisitAllNodes(const CXCursor& root, u32 level) {
    using namespace parsing_private;

    WrappedVisitChildren(root, [level](CXCursor cursor, CXCursor, CXClientData) {
        auto scratch = GetScratchArena();

        CXSourceLocation location = clang_getCursorLocation(cursor);
        if (clang_Location_isFromMainFile(location) == 0) {
            return CXChildVisit_Continue;
        }

        CXCursorKind cursor_kind = clang_getCursorKind(cursor);

        // Get location information
        unsigned int line, column;
        CXFile file;
        clang_getExpansionLocation(location, &file, &line, &column, nullptr);

        // Print cursor information
        std::cout << std::string(level, '-') << " " << GetCursorKindName(scratch.Arena, cursor_kind)
                  << " '" << GetCursorSpelling(scratch.Arena, cursor) << "' "
                  << "at line " << line << ":" << column;
        std::cout << "\n";

        // Visit all other children
        VisitAllNodes(cursor, level + 1);
        return CXChildVisit_Continue;
    });
}

void CollectKDKStructs(Arena* arena, const CXCursor& root, DynArray<StructInfo>* out) {
    using namespace parsing_private;

    auto visitor = [arena, out](CXCursor cursor,
                                CXCursor /* parent */,
                                CXClientData /* clientData */) -> CXChildVisitResult {
        CXSourceLocation location = clang_getCursorLocation(cursor);
        if (clang_Location_isFromMainFile(location) == 0) {
            return CXChildVisit_Continue;
        }

        CXCursorKind cursorKind = clang_getCursorKind(cursor);

        if (cursorKind == CXCursor_Namespace) {
            CollectKDKStructs(arena, cursor, out);
            // structs.insert(structs.end(), namespace_structs.begin(), namespace_structs.end());
        }

        if (cursorKind != CXCursor_StructDecl && cursorKind != CXCursor_ClassDecl) {
            return CXChildVisit_Continue;
        }

        FixedArray<const char*, 8> kdk_args = {};
        GetKDKAnnotationArgs(arena, cursor, &kdk_args);
        if (!kdk_args.IsEmpty()) {
            StructInfo info;
            info.Name = GetCursorSpelling(arena, cursor);
            info.Attributes = std::move(kdk_args);

            WrappedVisitChildren(cursor, [arena, &info](CXCursor c, CXCursor, CXClientData) {
                if (clang_getCursorKind(c) != CXCursor_FieldDecl) {
                    return CXChildVisit_Continue;
                }

                CXType clang_type = clang_getCursorType(c);
                const char* type_name = InternCXString(arena, clang_getTypeSpelling(clang_type));
                CXType canonical_type = clang_getCanonicalType(clang_type);
                const char* canonical_type_name =
                    InternCXString(arena, clang_getTypeSpelling(canonical_type));
                FieldInfo field = {
                    .Name = GetCursorSpelling(arena, c),
                    .TypeName = type_name,
                    .CanonicalTypeName = canonical_type_name,
                    // .ClangType = clang_type,
                };
                info.Fields.Push(field);
                return CXChildVisit_Continue;
            });

            out->Push(arena, std::move(info));
        }
        return CXChildVisit_Continue;
    };

    WrappedVisitChildren(root, visitor);
}

std::string generateImGuiCode(const StructInfo&) {
    std::string code;

#if 0

    // Add include guards
    code += "#pragma once\n\n";

    // Add includes
    code += "#include <imgui.h>\n";
    code += "#include <string>\n\n";

    // Add the BuildImgui function
    code += "// Auto-generated ImGui code for " + struct_info.Name + "\n";
    code += struct_info.Name + " BuildImgui_" + struct_info.Name + "() {\n";
    code += "    " + struct_info.Name + " result;\n\n";

    // Add ImGui widgets for each field
    for (const auto& field : struct_info.fields) {
        // Handle different types appropriately
        if (field.canonicalType == "int" || field.canonicalType == "long" ||
            field.canonicalType == "short" || field.canonicalType == "char") {
            code += "    ImGui::InputInt(\"" + field.Name + "\", &result." + field.Name + ");\n";
        } else if (field.canonicalType == "float") {
            code += "    ImGui::InputFloat(\"" + field.Name + "\", &result." + field.Name + ");\n";
        } else if (field.canonicalType == "double") {
            code += "    ImGui::InputDouble(\"" + field.Name + "\", &result." + field.Name + ");\n";
        } else if (field.canonicalType == "bool") {
            code += "    ImGui::Checkbox(\"" + field.Name + "\", &result." + field.Name + ");\n";
        } else if (field.canonicalType.find("std::string") != std::string::npos ||
                   field.canonicalType.find("std::basic_string") != std::string::npos) {
            code +=
                "    // Note: This is a simplified version, real implementation would need a char "
                "buffer\n";
            code += "    static char " + field.Name + "_buffer[256] = \"\";\n";
            code += "    ImGui::InputText(\"" + field.Name + "\", " + field.Name +
                    "_buffer, sizeof(" + field.Name + "_buffer));\n";
            code += "    result." + field.Name + " = " + field.Name + "_buffer;\n";
        } else if (field.type == "Vec3") {
            code +=
                "    ImGui::DragFloat3(\"" + field.Name + "\", &result." + field.Name + ".x);\n";
        } else {
            // For complex types, add a comment
            code += "    // TODO: Add appropriate ImGui widget for " + field.Name + " of type " +
                    field.canonicalType + "\n";
        }
    }
    code += "\n    return result;\n";
    code += "}\n";

#endif

    return code;
}

bool generateImGuiFile(const StructInfo& struct_info, const std::string& outputPath) {
    std::string filename = outputPath + "/imgui_" + struct_info.Name + ".h";

    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << generateImGuiCode(struct_info);
    file.close();

    return true;
}

}  // namespace kdk
