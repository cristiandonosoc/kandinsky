#pragma once

#include <kandinsky/entity.h>

namespace kdk {

// Macro-based reflection.

#define __COUNTER_MACRO(...) count++;
#define __ENUM_BUILDER_MACRO(FIELD_TYPE, FIELD_NAME, ...) FIELD_NAME,
#define __FIELD_DECLARATION_MACRO(FIELD_TYPE, FIELD_NAME, DEFAULT_VALUE, ...) \
    FIELD_TYPE _##FIELD_NAME = DEFAULT_VALUE;                                 \
    const FIELD_TYPE& Get##FIELD_NAME() const {                               \
        if (!_OverridenFields.test((i32)EField::FIELD_NAME)) [[likely]] {     \
            return _##FIELD_NAME;                                             \
        }                                                                     \
        return Archetype->_##FIELD_NAME;                                      \
    }
#define __SERDE_MACRO(FIELD_TYPE, FIELD_NAME, ...) \
    ::kdk::Serde(__sa, #FIELD_NAME, &__elem->_##FIELD_NAME);

#define FIELDS_ENTITY_FUNCTION(ENUM_NAME, FIELDS_MACRO) \
    consteval i32 __Get##ENUM_NAME##FieldCount() {      \
        i32 count = 0;                                  \
        FIELDS_MACRO(__COUNTER_MACRO)                   \
        return count;                                   \
    }

constexpr static i32 kMaxEntityFieldCount = 64;

struct EntityFieldMetadata {
    const char* FieldType = nullptr;
    i32 FieldTypeSize = 0;
    const char* FieldName = nullptr;
    const char* DefaultValueStr = nullptr;
    std::bitset<16> Flags = {};
};

struct EntityFieldMetadataHolder {
    EntityFieldMetadata FieldMetadatas[kMaxEntityFieldCount];
    i32 FieldCount = 0;
};

#define DECLARE_ENTITY(ENUM_NAME, FIELDS_MACRO)                            \
    FIELDS_ENTITY_FUNCTION(ENUM_NAME, FIELDS_MACRO);                       \
    struct ENUM_NAME##Entity {                                             \
        GENERATE_ENTITY(ENUM_NAME);                                        \
        constexpr static i32 kFieldCount = __Get##ENUM_NAME##FieldCount(); \
        static_assert(kFieldCount <= kMaxEntityFieldCount);                \
        static EntityFieldMetadataHolder gFieldsMetadata;                  \
        std::bitset<kFieldCount> _OverridenFields = {};                    \
        enum class EField : u8 {                                           \
            FIELDS_MACRO(__ENUM_BUILDER_MACRO)                             \
        };                                                                 \
        FIELDS_MACRO(__FIELD_DECLARATION_MACRO)                            \
    };

#define __GET_FLAG_ARG(_1, _2, _3, _4, ...) _4

#define __SET_ENTITY_METADATA_FLAGS_MACRO(...)                  \
    {                                                           \
        auto& metadata = metadata_holder.FieldMetadatas[index]; \
        metadata.Flags = __GET_FLAG_ARG(__VA_ARGS__, 0, 0, 0);  \
    }                                                           \
    index++;

#define __SET_ENTITY_METADATA_MACRO(FIELD_TYPE, FIELD_NAME, DEFAULT_VALUE, ...) \
    {                                                                           \
        auto& metadata = metadata_holder.FieldMetadatas[index];                 \
        metadata.FieldType = #FIELD_TYPE;                                       \
        metadata.FieldTypeSize = sizeof(FIELD_TYPE);                            \
        metadata.FieldName = #FIELD_NAME;                                       \
        metadata.DefaultValueStr = #DEFAULT_VALUE;                              \
    }                                                                           \
    index++;

// We do two passes to use the __VAR_ARGS__ trick to get the optional flags.
#define DEFINE_ENTITY(ENUM_NAME, FIELDS_MACRO)                                     \
    constexpr ::kdk::EntityFieldMetadataHolder __##ENUM_NAME##__SetEntityFlags() { \
        ::kdk::EntityFieldMetadataHolder metadata_holder = {};                     \
        metadata_holder.FieldCount = ENUM_NAME##Entity::kFieldCount;               \
        i32 index = 0;                                                             \
        FIELDS_MACRO(__SET_ENTITY_METADATA_MACRO);                                 \
        index = 0;                                                                 \
        FIELDS_MACRO(__SET_ENTITY_METADATA_FLAGS_MACRO);                           \
        return metadata_holder;                                                    \
    }                                                                              \
    ::kdk::EntityFieldMetadataHolder ENUM_NAME##Entity::gFieldsMetadata =          \
        __##ENUM_NAME##__SetEntityFlags();

constexpr u32 EntityFieldFlag_None = 0;
constexpr u32 EntityFieldFlag_NoOverridable = 1;
constexpr u32 EntityFieldFlag_NoSerialize = 2;
constexpr u32 EntityFieldFlag_ImGuiReadOnly = 3;

#define SERDE_ENTITY(SA, ELEMENT, FIELDS_MACRO) \
    SerdeArchive* __sa = SA;                    \
    auto* __elem = ELEMENT;                     \
    FIELDS_MACRO(__SERDE_MACRO)

}  // namespace kdk
