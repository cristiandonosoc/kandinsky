#pragma once

#include <kandinsky/core/defines.h>
#include <kandinsky/core/string.h>

#include <bit>
#include <limits>
#include <source_location>
#include <span>

namespace kdk {

struct PlatformState;

// System ------------------------------------------------------------------------------------------

constexpr u64 BYTE = 1;
constexpr u64 KILOBYTE = 1024 * BYTE;
constexpr u64 MEGABYTE = 1024 * KILOBYTE;
constexpr u64 GIGABYTE = 1024 * MEGABYTE;
constexpr u64 TERABYTE = 1024 * GIGABYTE;

// Arenas ------------------------------------------------------------------------------------------

enum class EArenaType : u8 {
    // Simple one buffer arena. Traps if the size is exceeded.
    FixedSize,
    // Starts as fixed size, but when the next allocation would overflow, it will allocate a new
    // arena of the same size and "chain" to it.
    // Note that uses some memory at the end of the buffer for the link data structure, so not all
    // reported memory is available.
    Extendable,
};
String ToString(EArenaType type);

struct Arena {
    u8* Start = nullptr;
    // NOTE: Shows the size of this particular arena.
    //       In the case of Extendable arenas, this represents only that "link".
    //       To know the total size of the arena, refer to |ExtendableData.TotalSize|.
    u64 Size = 0;
    u64 Offset = 0;

    EArenaType Type = EArenaType::FixedSize;

    struct Stats {
        u32 AllocCalls = 0;
        u32 FreeCalls = 0;
    } Stats = {};

    union {
        struct {
        } FixedData;
        struct {
            Arena* NextArena = nullptr;
            // The size of _this_ link.
            u64 MaxLinkOffset = 0;
            u64 TotalSize = 0;
        } ExtendableData;
    };

    FixedString<124> Name = {};

    Arena* GetPtr() { return this; }
};
bool IsValid(const Arena& arena);

Arena AllocateArena(String name, u64 size, EArenaType type = EArenaType::FixedSize);
void FreeArena(Arena* arena);

void ArenaReset(Arena* arena);
u8* ArenaPush(Arena* arena, u64 size, u64 alignment = 8);
u8* ArenaPushZero(Arena* arena, u64 size, u64 alignment = 8);

template <typename T>
T* ArenaPush(Arena* arena) {
    return (T*)ArenaPush(arena, sizeof(T), alignof(T));
}

template <typename T>
T* ArenaPushZero(Arena* arena) {
    return (T*)ArenaPushZero(arena, sizeof(T), alignof(T));
}

// Allocates memory and calls the struct initializer.
template <typename T>
T* ArenaPushInit(Arena* arena) {
    T* t = (T*)ArenaPushZero(arena, sizeof(T), alignof(T));
    new (t) T;  // Placement new.
    return t;
}

template <typename T>
[[nodiscard]] std::span<T> ArenaPushArray(Arena* arena, u64 count) {
    T* t = (T*)ArenaPush(arena, count * sizeof(T), alignof(T));
    return {t, count};
}

// Copy into the arena some data.
[[nodiscard]] inline std::span<u8> ArenaCopy(Arena* arena, std::span<u8> data, u64 alignment = 8) {
    u8* ptr = ArenaPush(arena, data.size_bytes(), alignment);
    if (ptr) {
        std::memcpy(ptr, data.data(), data.size_bytes());
    }
    return {ptr, data.size_bytes()};
}

// Non-copyable, Non-movable RAII style temporary arena.
// This is meant to be used in the scope of a stack frame only.
struct ScopedArena {
    Arena* Arena = nullptr;
    u64 OriginalOffset = 0;

    // Implicit converstion to Arena*
    operator struct Arena *() { return Arena; }

    explicit ScopedArena(struct Arena* arena, u64 original_offset);
    ~ScopedArena();

    ScopedArena(const ScopedArena&) = delete;
    ScopedArena& operator=(const ScopedArena&) = delete;

    ScopedArena(ScopedArena&&) = delete;
    ScopedArena& operator=(ScopedArena&&) = delete;
};
inline ScopedArena GetScopedArena(Arena* arena) { return ScopedArena(arena, arena->Offset); }

ScopedArena GetScratchArena(Arena* conflict1 = nullptr, Arena* conflict2 = nullptr);

// Use for testing.
std::span<Arena> ReferenceScratchArenas();

struct BlockMetadata {
    std::source_location SourceLocation = {};
};

struct BlockHandle {
    // 8 bit is block-shift size, 24 is for index.
    u32 _Value = 0;

    u32 GetBlockShift() const { return (_Value >> 24) & 0xFF; }
    u32 GetBlockIndex() const { return _Value & 0x00FFFFFF; }
};

inline bool IsValid(const BlockHandle& handle) { return handle._Value != 0; }

struct BlockAllocationResult {
    BlockHandle Handle = {};
    std::span<u8> Memory = {};
    BlockMetadata* BlockMetadata = nullptr;
};

template <u32 BLOCK_SIZE, u32 BLOCK_COUNT>
struct BlockArena {
    static constexpr u64 kTotalSize = BLOCK_SIZE * BLOCK_COUNT;
    static constexpr u64 kBlockSize = BLOCK_SIZE;
    static constexpr u64 kBlockCount = BLOCK_COUNT;
    static constexpr u64 kBlockShift = std::countr_zero(BLOCK_SIZE);

    Array<Array<u8, BLOCK_SIZE>, BLOCK_COUNT> Blocks = {};
    Array<BlockMetadata, BLOCK_COUNT> BlocksMetadata = {};
    Array<u32, BLOCK_COUNT> BlocksFreeList = {};

    FixedString<124> Name = {};
    u32 NextFreeBlock = 0;
    struct {
        i64 TotalAllocCalls = 0;
        i32 AllocatedBlocks = 0;
    } Metadata;

    void Init(String name);
    void Shutdown() {}
    BlockAllocationResult AllocateBlock(
        std::source_location source_location = std::source_location::current());
    bool FreeBlock(BlockHandle handle);
};

// clang-format off
static_assert(BlockArena<  1 * KILOBYTE, 1024>::kBlockShift == 10);
static_assert(BlockArena<  4 * KILOBYTE, 1024>::kBlockShift == 12);
static_assert(BlockArena< 16 * KILOBYTE, 1024>::kBlockShift == 14);
static_assert(BlockArena< 64 * KILOBYTE, 1024>::kBlockShift == 16);
static_assert(BlockArena<256 * KILOBYTE, 1024>::kBlockShift == 18);
static_assert(BlockArena<  1 * MEGABYTE, 1024>::kBlockShift == 20);
// clang-format on

template <u32 BLOCK_SIZE, u32 BLOCK_COUNT>
void BlockArena<BLOCK_SIZE, BLOCK_COUNT>::Init(String name) {
    Name = name;
    for (u32 i = 0; i < BLOCK_COUNT; i++) {
        BlocksFreeList[i] = i + 1;
    }
    BlocksFreeList[BLOCK_COUNT - 1] = std::numeric_limits<u32>::max();
    NextFreeBlock = 0;
    Metadata = {};
}

template <u32 BLOCK_SIZE, u32 BLOCK_COUNT>
BlockAllocationResult BlockArena<BLOCK_SIZE, BLOCK_COUNT>::AllocateBlock(
    std::source_location source_location) {
    // Check if there are free blocks.
    if (NextFreeBlock == std::numeric_limits<u32>::max()) {
        return {};
    }

    // Find the next free block index.
    u32 block_index = NextFreeBlock;
    NextFreeBlock = BlocksFreeList[block_index];
    BlocksFreeList[block_index] = std::numeric_limits<u32>::max();
    ASSERT(block_index < BLOCK_COUNT);
    ASSERT(block_index < (1 << 24));

    BlockHandle handle = {};
    handle._Value = (kBlockShift << 24) | block_index;

    std::span<u8> block_span = Blocks[block_index].ToSpan();
    BlockMetadata* metadata = &BlocksMetadata[block_index];
    metadata->SourceLocation = std::move(source_location);

    Metadata.AllocatedBlocks++;
    Metadata.TotalAllocCalls++;

    return {handle, block_span, metadata};
}

template <u32 BLOCK_SIZE, u32 BLOCK_COUNT>
bool BlockArena<BLOCK_SIZE, BLOCK_COUNT>::FreeBlock(BlockHandle handle) {
    ASSERT(IsValid(handle));

    u32 block_shift = handle.GetBlockShift();
    ASSERT(block_shift == kBlockShift);

    u32 block_index = handle.GetBlockIndex();
    ASSERT(block_index < BLOCK_COUNT);

    // Add the block back to the free list.
    BlocksFreeList[block_index] = NextFreeBlock;
    NextFreeBlock = block_index;

    Metadata.AllocatedBlocks--;
    ASSERT(Metadata.AllocatedBlocks >= 0);

    return true;
}

// Format: (SIZE_NAME, BLOCK_SIZE, BLOCK_COUNT, BLOCK_SHIFT)
// clang-format off
#define BLOCK_ARENA_TYPES(X)			\
    X( 1KB,  1 * KILOBYTE, (u32)1 << 10, 10)	\
    X( 4KB,  4 * KILOBYTE, (u32)1 << 10, 12)	\
    X(16KB, 16 * KILOBYTE, (u32)1 << 10, 14)
// clang-format on

struct BlockArenaManager {
#define X(SIZE_NAME, BLOCK_SIZE, BLOCK_COUNT, ...) \
    BlockArena<BLOCK_SIZE, BLOCK_COUNT>* _BlockArena_##SIZE_NAME = nullptr;
    BLOCK_ARENA_TYPES(X)
#undef X
};

void Init(BlockArenaManager* bam);
void Shutdown(BlockArenaManager* bam);

BlockAllocationResult AllocateBlock(
    BlockArenaManager* bam,
    u32 byte_size,
    std::source_location source_location = std::source_location::current());
bool FreeBlock(BlockArenaManager* bam, BlockHandle handle);

// Memory Alignment --------------------------------------------------------------------------------

inline bool IsPowerOf2(u64 a) { return ((a & (a - 1)) == 0); }
void* Align(void* ptr, u64 alignment);
void* AlignForward(void* ptr, u64 alignment);

// UTILITIES ---------------------------------------------------------------------------------------

String ToMemoryString(Arena* arena, u64 bytes);

}  // namespace kdk
