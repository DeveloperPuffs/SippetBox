#include <engine/platform.h>
#include <engine/logging.h>
#include <engine/engine.h>

#ifdef PLATFORM_APPLE
#include <engine/cocoa.h>
#else
#include <SDL3/SDL.h>
#endif

bool initializeEngine(void) {
#ifdef PLATFORM_APPLE
  initializeCocoa();
#else
  if (!SDL_Init(SDL_INIT_VIDEO))
    return (logError("Failed to initialize SDL: %s", SDL_GetError()), terminate(&state, 1));

  if (!SDL_Vulkan_LoadLibrary(NULL))
    return (logError("Failed to load Vulkan library: %s", SDL_GetError()), terminate(&state, 1));
#endif

  return true;
}

bool updateEngine(void) {
#ifdef PLATFORM_APPLE
  updateCocoa();
#else
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) return false;
  }
#endif

  return true;
}

void terminateEngine(void) {
#ifdef PLATFORM_APPLE
  terminateCocoa();
#else
  SDL_Vulkan_UnloadLibrary();
  SDL_Quit();
#endif
}