#pragma once

#include <kandinsky/core/container.h>
#include <kandinsky/core/string.h>

#include <source_location>

namespace kdk {

struct PlatformState;

// The schedule system is meant for "scheduling" little callbacks to be run later for some time,
// normally every frame.
struct ScheduleSystem {
    using Task = Function<void(PlatformState* ps)>;

    struct Entry {
        double StartTime = 0.0;
        double Duration = 0.0;

        Task Task;
		FixedString<32> Name;
		std::source_location SourceLocation;

        bool operator<(const Entry& other) const;
    };

    FixedVector<Entry, 256> Entries;

    PlatformState* OwningPlatformState = nullptr;
};

void Schedule(ScheduleSystem* ss,
			  String name,
              double duration,
              ScheduleSystem::Task&& task,
              const std::source_location& location = std::source_location::current());

void Init(PlatformState* ps, ScheduleSystem* ss);
void Shutdown(ScheduleSystem* ss);
void Update(ScheduleSystem* ss);
void BuildImGui(ScheduleSystem* ss);

}  // namespace kdk
