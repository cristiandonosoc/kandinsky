#pragma once

#include <kandinsky/defines.h>

namespace kdk {

struct PlatformState;

constexpr u64 BYTE = 1;
constexpr u64 KILOBYTE = 1024 * BYTE;
constexpr u64 MEGABYTE = 1024 * KILOBYTE;
constexpr u64 GIGABYTE = 1024 * MEGABYTE;
constexpr u64 TERABYTE = 1024 * GIGABYTE;

struct Arena {
    u8* Start = nullptr;
    u64 Size = 0;
    u64 Offset = 0;
};
bool IsValid(const Arena& arena);

Arena AllocateArena(u64 size);
void FreeArena(Arena* arena);

inline void ArenaReset(Arena* arena) { arena->Offset = 0; }
u8* ArenaPush(Arena* arena, u64 size, u64 aligment = 8);
u8* ArenaPushZero(Arena* arena, u64 size, u64 aligment = 8);

// String specific.
const char* InternString(Arena* arena, const char* string);

// System.

bool InitMemory(PlatformState* ps);
void ShutdownMemory(PlatformState* ps);

}  // namespace kdk
