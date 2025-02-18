#include <SDL3/SDL_time.h>
#include <kandinsky/time.h>

#include <SDL3/SDL_timer.h>

#include <cassert>
#include <format>

namespace kdk {

double GetSeconds() { return ((double)SDL_GetTicksNS()) / 1'000'000'000.0; }

std::string PrintAsDate(SDL_Time time) {
    SDL_DateTime datetime;
    bool ok = SDL_TimeToDateTime(time, &datetime, true);
    assert(ok);

    return std::format("{:04d}/{:02d}/{:02d} {:02d}:{:02d}:{:02d}",
                       datetime.year,
                       datetime.month,
                       datetime.day,
                       datetime.hour,
                       datetime.minute,
                       datetime.second);
}

}  // namespace kdk
