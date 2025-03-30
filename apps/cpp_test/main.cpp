#include <kandinsky/memory.h>
#include <kandinsky/string.h>

// #include <b64.h>

#include <iterator>

struct Foo {
    int Bar = 0;
};

struct Test {
    Foo* Foos[4] = {};
};

struct TT {
	union {
		u32 Data = 0;
		struct {
			u8 Pad1;
			u8 Pad2;
			u8 Pad3;
			bool Dirty: 1;
		} Flags;
	};

	void SetParentID(u32 id) { Data = (Data & 0xFF000000) | (id & 0x00FFFFFF); }
	u32 GetParentID() const { return Data & 0x00FFFFFF; }
	void Print() const {
		printf("Dirty: %d, ID: %u\n", Flags.Dirty, GetParentID());
	}
};
static_assert(sizeof(TT) == 4);

int main() {
    using namespace kdk;
    Foo foo1;
    Foo foo2;

    {
        Test test;
        for (size_t i = 0; i < std::size(test.Foos); i++) {
            printf("%llu: %p\n", i, (void*)test.Foos[i]);
        }
    }

    {
        Test test = {
            .Foos =
                {
                       &foo1,
                       &foo2,
                       },
        };
        for (size_t i = 0; i < std::size(test.Foos); i++) {
            printf("%llu: %p\n", i, (void*)test.Foos[i]);
        }
    }

    Arena arena = AllocateArena(1 * MEGABYTE);

    const char* path = "assets/models";
    if (auto result = paths::ListDir(&arena, String(path)); IsValid(result)) {
        for (u32 i = 0; i < result.Size; i++) {
            printf("- %s\n", result.Entries[i].Path.Str());
        }
    }

	TT tt = {};
	tt.Print();

	tt.Flags.Dirty = true;
	tt.Print();

	tt.SetParentID(22);
	tt.Print();

	/* u32 v = 0xFF223344; */
	/* printf("0x%08X\n", v); */
	/* printf("0x%08X\n", v >> 8); */
	/* printf("0x%08X\n", v >> 16); */
	/* printf("0x%08X\n", v >> 24); */
}
