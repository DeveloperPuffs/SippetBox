#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <engine/logging.h>
#include <engine/graphics.h>

struct State {
  struct Application *application;
  struct Window *window;
};

static int terminate(struct State *const state, const int exitCode);

int main() {
  struct State state = {};
  state.application = NULL;
  state.window = NULL;

  if (!SDL_Init(SDL_INIT_VIDEO))
    return (logError("Failed to initialize SDL: %s", SDL_GetError()), terminate(&state, 1));

  if (!SDL_Vulkan_LoadLibrary(NULL))
    return (logError("Failed to load Vulkan library: %s", SDL_GetError()), terminate(&state, 1));

  if (!(state.application = createApplication("SippetBox")))
    return (logError("Failed to create application"), terminate(&state, 1));

  if (!(state.window = createWindow(state.application, "SippetBox", 640, 480)))
    return (logError("Failed to create window"), terminate(&state, 1));

  while (true) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) return terminate(&state, 0);
    }

    drawSomething(state.window);

    if (!updateWindow(state.window))
      return (logError("Failed to update window"), terminate(&state, 1));
  }

  return terminate(&state, 0);
}

static int terminate(struct State *const state, const int exitCode) {
  destroyWindow(state->window);
  destroyApplication(state->application);
  SDL_Vulkan_UnloadLibrary();
  SDL_Quit();

  return exitCode;
}