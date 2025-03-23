#pragma once

#include <string>
#include <vector>

#include <clang-c/Index.h>

struct FieldInfo {
    std::string name;
    std::string type;
    std::string canonicalType;
};

struct StructInfo {
    std::string name;
    std::vector<std::string> kdkAttributes;
    std::vector<FieldInfo> fields;
};

// Function declarations
std::vector<StructInfo> collectKDKStructs(const CXCursor& rootCursor);
std::vector<std::string> getKDKAnnotationArgs(const CXCursor& cursor);
std::string getCursorSpelling(CXCursor cursor);
std::string getCursorKindName(CXCursorKind cursorKind); 