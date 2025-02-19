#pragma once

#include <kandinsky/defines.h>

namespace kdk {

struct PlatformState;

constexpr u64 BYTE = 1;
constexpr u64 KILOBYTE = 1024 * BYTE;
constexpr u64 MEGABYTE = 1024 * KILOBYTE;
constexpr u64 GIGABYTE = 1024 * MEGABYTE;
constexpr u64 TERABYTE = 1024 * GIGABYTE;

enum class EArenaType : u8 {
	FixedSize,
	Extendable,
};

struct Arena {
    u8* Start = nullptr;
    u64 Size = 0;
    u64 Offset = 0;

	EArenaType Type = EArenaType::FixedSize;

	union {
		struct {
			Arena* NextArena = nullptr;
			// The size of _this_ link.
			u64 MaxSize = 0;
		} ExtendableData;
	};
};
bool IsValid(const Arena& arena);


Arena AllocateArena(u64 size, EArenaType type = EArenaType::FixedSize);
void FreeArena(Arena* arena);

inline void ArenaReset(Arena* arena) { arena->Offset = 0; }
u8* ArenaPush(Arena* arena, u64 size, u64 aligment = 8);
u8* ArenaPushZero(Arena* arena, u64 size, u64 aligment = 8);

// String specific.
const char* InternString(Arena* arena, const char* string);

// System.

bool InitMemory(PlatformState* ps);
void ShutdownMemory(PlatformState* ps);

// Memory Alignment --------------------------------------------------------------------------------

inline bool IsPowerOf2(u64 a) { return ((a & (a - 1)) == 0); }
void* Align(void* ptr, u64 aligment);
void* AlignForward(void* ptr, u64 aligment);

}  // namespace kdk
