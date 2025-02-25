#include <kandinsky/debug.h>

#include <kandinsky/math.h>
#include <kandinsky/opengl.h>

#include <SDL3/SDL_log.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <dbghelp.h>

namespace kdk {

bool Debug::Init(PlatformState* ps) {
    if (ps->DebugLineBatcher) {
        SDL_Log("ERROR: DebugLineBatcher already set");
        return false;
    }

    if (FindLineBatcher(&ps->LineBatchers, "DebugLineBatcher")) {
        SDL_Log("ERROR: DebugLineBatcher already found");
        return false;
    }

    ps->DebugLineBatcher = CreateLineBatcher(ps, &ps->LineBatchers, "DebugLineBatcher");

    return true;
}

void Debug::Shutdown(PlatformState* ps) {
    assert(IsValid(*ps->DebugLineBatcher));
    ps->DebugLineBatcher = nullptr;
}

void Debug::StartFrame(PlatformState* ps) {
    assert(IsValid(*ps->DebugLineBatcher));

    Reset(ps->DebugLineBatcher);
}

void Debug::Render(PlatformState* ps, const Shader& shader, const glm::mat4& view_proj) {
    // TODO(cdc): Maybe detect differences so that we don't send redundant data every frame.
    Buffer(ps, *ps->DebugLineBatcher);

    Use(shader);
    SetMat4(shader, "uViewProj", glm::value_ptr(view_proj));
    Draw(*ps->DebugLineBatcher, shader);
}

void Debug::DrawLines(PlatformState* ps,
                      std::span<std::pair<glm::vec3, glm::vec3>> points,
                      Color32 color,
                      float line_width) {
    StartLineBatch(ps->DebugLineBatcher, GL_LINES, color, line_width);

    for (const auto& [p1, p2] : points) {
        AddPoints(ps->DebugLineBatcher, p1, p2);
    }

    EndLineBatch(ps->DebugLineBatcher);
}

void Debug::DrawArrow(PlatformState* ps,
                      const glm::vec3& start,
                      const glm::vec3& end,
                      Color32 color,
                      float arrow_size,
                      float line_width) {
    // Find the axis vectors.
    glm::vec3 direction = glm::normalize(end - start);
    glm::vec3 up(0, 1, 0);
    glm::vec3 right = glm::normalize(glm::cross(up, direction));
    up = glm::normalize(glm::cross(direction, right));

    glm::vec3 front_offset = direction * arrow_size;
    glm::vec3 right_offset = right * arrow_size;
    glm::vec3 up_offset = up * arrow_size;

    StartLineBatch(ps->DebugLineBatcher, GL_LINES, color, line_width);
    AddPoints(ps->DebugLineBatcher, start, end);

    AddPoints(ps->DebugLineBatcher, end, end - front_offset - right_offset - up_offset);
    AddPoints(ps->DebugLineBatcher, end, end - front_offset - right_offset + up_offset);
    AddPoints(ps->DebugLineBatcher, end, end - front_offset + right_offset - up_offset);
    AddPoints(ps->DebugLineBatcher, end, end - front_offset + right_offset + up_offset);

    EndLineBatch(ps->DebugLineBatcher);
}

void Debug::DrawSphere(PlatformState* ps,
                       const glm::vec3& center,
                       float radius,
                       u32 segments,
                       Color32 color,
                       float line_width) {
    // Need at least 4 segments
    segments = glm::max(segments, (u32)4);

    const float angle_inc = 2.f * PI / segments;
    u32 num_segments_y = segments / 2;
    float latitude = angle_inc;
    float siny1 = 0.0f, cosy1 = 1.0f;

    StartLineBatch(ps->DebugLineBatcher, GL_LINES, color, line_width);

    while (num_segments_y--) {
        const float siny2 = glm::sin(latitude);
        const float cosy2 = glm::cos(latitude);

        glm::vec3 vertex1 = glm::vec3(siny1, cosy1, 0.0f) * radius + center;
        glm::vec3 vertex3 = glm::vec3(siny2, cosy2, 0.0f) * radius + center;
        float longitude = angle_inc;

        u32 num_segments_x = segments;
        while (num_segments_x--) {
            const float sinx = glm::sin(longitude);
            const float cosx = glm::cos(longitude);

            glm::vec3 vertex2 = glm::vec3((cosx * siny1), cosy1, (sinx * siny1)) * radius + center;
            glm::vec3 vertex4 = glm::vec3((cosx * siny2), cosy2, (sinx * siny2)) * radius + center;

            AddPoints(ps->DebugLineBatcher, vertex1, vertex2);
            AddPoints(ps->DebugLineBatcher, vertex1, vertex3);

            vertex1 = vertex2;
            vertex3 = vertex4;
            longitude += angle_inc;
        }
        siny1 = siny2;
        cosy1 = cosy2;
        latitude += angle_inc;
    }

    EndLineBatch(ps->DebugLineBatcher);
}

void Debug::DrawCone(PlatformState* ps,
                     const glm::vec3& origin,
                     const glm::vec3& direction,
                     float length,
                     float angle,
                     u32 segments,
                     Color32 color,
                     float line_width) {
    const glm::vec3 front = glm::normalize(direction);

    // If the cone is looking exactly up or down (which can be common), we change the up axis to
    // look "up" the x axis.
    glm::vec3 up(0, 1, 0);
    if (Math::Equals(glm::abs(glm::dot(front, up)), 1.0f)) {
        up = {1, 0, 0};
    }

    const glm::vec3 end = origin + front * length;

    // Determine the axis.
    glm::vec3 right = glm::normalize(glm::cross(up, front));
    up = glm::normalize(glm::cross(front, right));

    // Need at least 4 sides
    segments = glm::max(segments, (u32)4);
    u32 num_segments = segments;

    const float radius = length * glm::tan(angle);
    const glm::vec3 rright = right * radius;
    const glm::vec3 rup = up * radius;

    float sin1 = 0.0f;
    float cos1 = 1.0f;

    // TODO(cdc): This could be improved to use GL_LINE_STRIP for the circle.
    StartLineBatch(ps->DebugLineBatcher, GL_LINES, color, line_width);

    const float circle_angle_inc = 2.0f * PI / segments;
    float circle_angle = circle_angle_inc;
    while (num_segments--) {
        const float sin2 = glm::sin(circle_angle);
        const float cos2 = glm::cos(circle_angle);

        glm::vec3 v1 = end + rright * sin1 + rup * cos1;
        glm::vec3 v2 = end + rright * sin2 + rup * cos2;

        AddPoints(ps->DebugLineBatcher, v1, v2);
        AddPoints(ps->DebugLineBatcher, origin, v2);

        sin1 = sin2;
        cos1 = cos2;
        circle_angle += circle_angle_inc;
    }

    EndLineBatch(ps->DebugLineBatcher);
}

void Debug::PrintBacktrace(Arena* arena, u32 frames_to_skip) {
    // We don't want to get this function in the way.
    frames_to_skip++;

    constexpr u32 kFramesToCapture = 16;
    std::array<void*, kFramesToCapture> frames;
    u32 frame_count = CaptureStackBackTrace(frames_to_skip, 16, frames.data(), NULL);

    HANDLE handle = GetCurrentProcess();
    if (!SymInitialize(handle, nullptr, true)) {
        SDL_Log("ERROR: PrintBacktrace: SymInitialize: %s\n", GetWindowsErrorCode(arena));
        return;
    }

    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

    // Symbol info buffer.
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)ArenaPushZero(arena, sizeof(SYMBOL_INFO) + 256);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    IMAGEHLP_LINE64 line = {};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    SDL_Log("--- BACKTRACE --------------------------------------------------------------------\n");

    bool has_seen_valid_frame = false;
    u32 frames_skipped = 0;
    for (u32 i = 0; i < frame_count; i++) {
        u64 addr = (u64)frames[i];

        const char* function_name = nullptr;
        if (SymFromAddr(handle, addr, nullptr, symbol)) {
            function_name = symbol->Name;
        }

        if (!function_name) {
            // We don't want to show a bunch of "<unknown>" at the top if we can avoid it.
            if (has_seen_valid_frame) {
                SDL_Log("Frame %02d: <unknown>\n", i);
            } else {
                frames_skipped++;
            }
            continue;
        }

        if (!has_seen_valid_frame) {
            SDL_Log("Skipped %d frames (were \"<unknown>\")\n", frames_skipped);
            has_seen_valid_frame = true;
        }

        // Get the file and line info.
        const char* file = "<unknown>";
        u32 line_number = 0;
        DWORD displacement = 0;
        if (SymGetLineFromAddr64(handle, addr, &displacement, &line)) {
            file = line.FileName;
            line_number = (u32)line.LineNumber;

            // Remove bazel nonesense.
            const char* bazel_marker = "_main\\";
            if (const char* marker_pos = strstr(file, bazel_marker)) {
                file = marker_pos + strlen(bazel_marker);
            }
        }

        SDL_Log("Frame %02d: %s (%s:%d)\n", i - frames_skipped, function_name, file, line_number);
    }
}

const char* Debug::GetWindowsErrorCode(Arena* arena) {
    char* buffer = (char*)ArenaPush(arena, 256);

    // Format the error message
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   GetLastError(),
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   buffer,
                   256,
                   NULL);

    return buffer;
}

}  // namespace kdk
