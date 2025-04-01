#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <engine/engine.h>
#include <engine/logging.h>
#include <engine/platform.h>
#include <engine/graphics.h>

struct State {
  struct Application *application;
  struct Window *window;
};

static int terminate(struct State *const state, const int exitCode);

int main(void) {
  if (!initializeEngine()) return 1;

  struct State state = {};
  state.application = NULL;
  state.window = NULL;

  if (!(state.application = createApplication("SippetBox")))
    return (logError("Failed to create application"), terminate(&state, 1));
  
  logDebug("Created Application");

  if (!(state.window = createWindow(state.application, "SippetBox", 640, 480)))
    return (logError("Failed to create window"), terminate(&state, 1));

  while (updateEngine() && !windowShouldClose(state.window)) {
    drawSomething(state.window);
    updateWindow(state.window);
  }

  return terminate(&state, 0);
}

static int terminate(struct State *const state, const int exitCode) {
  destroyWindow(state->window);
  destroyApplication(state->application);
  terminateEngine();
  return exitCode;
}