#pragma once

#include <SDL3/SDL_stdinc.h>

#include <string>

namespace kdk {

constexpr double SECOND = 1;
constexpr double MILLISECOND = SECOND / 1000.0;
constexpr double MICROSECOND = MILLISECOND / 1000.0;
constexpr double NANOSECOND = MICROSECOND / 1000.0;

double GetSeconds();

std::string PrintAsDate(SDL_Time time);

}  // namespace kdk
