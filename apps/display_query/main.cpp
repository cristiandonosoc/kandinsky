#include <SDL3/SDL.h>

#include <SDL3/SDL_video.h>

int main() {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("ERROR: Initializing SDL: %s\n", SDL_GetError());
		return -1;
	}

	int display_count = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&display_count);
	if (!displays) {
		SDL_Log("ERROR: GetDisplays: %s\n", SDL_GetError());
		return -1;
	}

	for (int i = 0; i < display_count; i++) {
		SDL_DisplayID display_id = displays[i];

		const char* display_name = SDL_GetDisplayName(display_id);
		SDL_Log("Display %d: %s\n", i, display_name);

		SDL_Rect bounds = {};
		if (SDL_GetDisplayBounds(display_id, &bounds)) {
			SDL_Log("- Bounds: (%d, %d)\n", bounds.x, bounds.y);
		}
	}

	return 0;
}
