#include <sippet/core/platform.h>
#include <sippet/core/logging.h>
#include <sippet/sippet.h>

#ifdef SIPPET_PLATFORM_APPLE
#include "cocoa.h"
#else
#include <SDL3/SDL.h>
#endif

bool sippetInitialize(void) {
#ifdef SIPPET_PLATFORM_APPLE
  sippetInitializeCocoa();
#else
  if (!SDL_Init(SDL_INIT_VIDEO))
    return (sippetLogError("Failed to initialize SDL: %s", SDL_GetError()), terminate(&state, 1));

  if (!SDL_Vulkan_LoadLibrary(NULL))
    return (sippetLogError("Failed to load Vulkan library: %s", SDL_GetError()), terminate(&state, 1));
#endif

  return true;
}

bool sippetUpdate(void) {
#ifdef SIPPET_PLATFORM_APPLE
  sippetUpdateCocoa();
#else
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) return false;
  }
#endif

  return true;
}

void sippetTerminate(void) {
#ifdef SIPPET_PLATFORM_APPLE
  sippetTerminateCocoa();
#else
  SDL_Vulkan_UnloadLibrary();
  SDL_Quit();
#endif
}