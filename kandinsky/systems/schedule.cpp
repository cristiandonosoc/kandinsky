#include <imgui.h>
#include <kandinsky/systems/schedule.h>

#include <kandinsky/core/algorithm.h>
#include <kandinsky/platform.h>

namespace kdk {

bool ScheduleSystem::Entry::operator<(const Entry& other) const {
    if (StartTime != other.StartTime) {
        return StartTime < other.StartTime;
    }

    return Duration < other.Duration;
}

void Schedule(ScheduleSystem* ss,
              String name,
              double duration,
              ScheduleSystem::Task&& task,
              const std::source_location& location) {
    ASSERT(duration > 0.0);
    double now = ss->OwningPlatformState->CurrentTimeTracking->TotalSeconds;

    ss->Entries.Push({
        .StartTime = now,
        .Duration = duration,
        .Task = std::move(task),
        .Name = name,
        .SourceLocation = location,
    });

    ss->Entries.Sort();
}

bool Init(PlatformState* ps, ScheduleSystem* ss) {
    ss->OwningPlatformState = ps;
    ss->Entries.Clear();
    SDL_Log("Initialized ScheduleSystem");
    return true;
}

void Shutdown(ScheduleSystem* ss) {
    ResetStruct(ss);
    SDL_Log("Shutdown ScheduleSystem");
}

void Start(ScheduleSystem* ss) {
    ResetStruct(&ss->Entries);
    SDL_Log("Started ScheduleSystem");
}
void Stop(ScheduleSystem* ss) {
    ResetStruct(&ss->Entries);
    SDL_Log("Stopped ScheduleSystem");
}

void Update(ScheduleSystem* ss) {
    double now = ss->OwningPlatformState->CurrentTimeTracking->TotalSeconds;

    for (auto& entry : ss->Entries) {
        if (now < entry.StartTime) {
            // No entry schedule yet, bail out.
            break;
        }

        // If we're within the end time, we run it.
        double end_time = entry.StartTime + entry.Duration;
        if (now < end_time) {
            entry.Task(ss->OwningPlatformState);
        }
    }

    // Remove all tasks that have completed.
    ss->Entries.RemoveAllPred([now](const ScheduleSystem::Entry& entry) {
        double end_time = entry.StartTime + entry.Duration;
        return now > end_time;
    });
}

void BuildImGui(ScheduleSystem* ss) {
    double now = ss->OwningPlatformState->CurrentTimeTracking->TotalSeconds;

    if (ImGui::BeginListBox(" ", ImVec2(-FLT_MIN, -FLT_MIN))) {
        for (const auto& entry : ss->Entries) {
            ImGui::Text("%s: %.2fs", entry.Name.Str(), entry.Duration);
            ImGui::SameLine();

            double progress;
            if (now < entry.StartTime) {
                progress = 1.0f;
            } else if (double end_time = entry.StartTime + entry.Duration; now < end_time) {
                progress = (end_time - now) / entry.Duration;
            } else {
                progress = 0.0f;
            }

            ImGui::ProgressBar((float)progress);
            if (ImGui::IsItemHovered()) {
                auto scratch = GetScratchArena();
                String location = ToString(scratch, entry.SourceLocation);

                ImGui::BeginTooltip();
                ImGui::Text("%s", location.Str());
                ImGui::EndTooltip();
            }
        }
        ImGui::EndListBox();
    }
}

}  // namespace kdk
