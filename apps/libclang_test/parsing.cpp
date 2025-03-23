#include "parsing.h"

#include <clang-c/Index.h>
#include <functional>

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

std::vector<std::string> getKDKAnnotationArgs(const CXCursor& cursor) {
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

// Modify the memberVisitor to collect fields instead of printing them
CXChildVisitResult memberVisitor(CXCursor cursor, CXCursor /* parent */, CXClientData clientData) {
    CXSourceLocation location = clang_getCursorLocation(cursor);
    if (clang_Location_isFromMainFile(location) == 0) {
        return CXChildVisit_Continue;
    }

    CXCursorKind cursorKind = clang_getCursorKind(cursor);
    if (cursorKind == CXCursor_FieldDecl) {
        auto fields = reinterpret_cast<std::vector<FieldInfo>*>(clientData);

        CXType fieldType = clang_getCursorType(cursor);
        CXString typeSpelling = clang_getTypeSpelling(fieldType);
        CXType canonicalType = clang_getCanonicalType(fieldType);
        CXString canonicalSpelling = clang_getTypeSpelling(canonicalType);

        FieldInfo field{getCursorSpelling(cursor),
                       clang_getCString(typeSpelling),
                       clang_getCString(canonicalSpelling)};

        fields->push_back(std::move(field));

        clang_disposeString(canonicalSpelling);
        clang_disposeString(typeSpelling);
    }

    return CXChildVisit_Continue;
}

std::vector<StructInfo> collectKDKStructs(const CXCursor& rootCursor) {
    std::vector<StructInfo> structs;

    auto visitor = [&structs](CXCursor cursor, CXCursor /* parent */, CXClientData /* clientData */) -> CXChildVisitResult {
        CXSourceLocation location = clang_getCursorLocation(cursor);
        if (clang_Location_isFromMainFile(location) == 0) {
            return CXChildVisit_Continue;
        }

        CXCursorKind cursorKind = clang_getCursorKind(cursor);

        if (cursorKind == CXCursor_Namespace) {
            auto namespace_structs = collectKDKStructs(cursor);
            structs.insert(structs.end(), namespace_structs.begin(), namespace_structs.end());
        }

        if (cursorKind != CXCursor_StructDecl && cursorKind != CXCursor_ClassDecl) {
            return CXChildVisit_Continue;
        }

        std::vector<std::string> kdkArgs = getKDKAnnotationArgs(cursor);
        if (!kdkArgs.empty()) {
            StructInfo info;
            info.name = getCursorSpelling(cursor);
            info.kdkAttributes = std::move(kdkArgs);

            // Collect fields
            clang_visitChildren(cursor, memberVisitor, &info.fields);

            structs.push_back(std::move(info));
        }
        return CXChildVisit_Continue;
    };

    clang_visitChildren(rootCursor,
                       [](CXCursor c, CXCursor p, CXClientData d) -> CXChildVisitResult {
                           return (*static_cast<decltype(visitor)*>(d))(c, p, nullptr);
                       },
                       &visitor);
    return structs;
} 