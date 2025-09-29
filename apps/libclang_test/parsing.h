#pragma once

#include <kandinsky/core/container.h>
#include <kandinsky/core/memory.h>

#include <clang-c/Index.h>

#include <string>
#include <vector>

namespace kdk {

struct FieldInfo {
    String Name;
    String TypeName;
    String CanonicalTypeName;
    // CXType ClangType;
};

struct StructInfo {
    String Name;

    FixedVector<String, 8> Attributes;
    FixedVector<FieldInfo, 64> Fields;
};

void VisitAllNodes(const CXCursor& root, u32 level = 0);

// Function declarations
void CollectKDKStructs(Arena* arena, const CXCursor& root, DynArray<StructInfo>* out);

// Generate ImGui code for a struct
std::string generateImGuiCode(const StructInfo& structInfo);

// Generate ImGui file for a struct
bool generateImGuiFile(const StructInfo& structInfo, const std::string& outputPath);

}  // namespace kdk
