#include <stddef.h>
#include <stdbool.h>
#include <sippet/core/logging.h>
#include <sippet/core/platform.h>
#include <sippet/graphics.h>
#include <sippet/sippet.h>

struct State {
  struct SippetApplication *application;
  struct SippetWindow *window;
};

static int terminate(struct State *const state, const int exitCode);

int main(void) {
  if (!sippetInitialize()) return 1;

  struct State state = {};
  state.application = NULL;
  state.window = NULL;

  if (!(state.application = sippetCreateApplication("SippetBox")))
    return (sippetLogError("Failed to create application"), terminate(&state, 1));

  if (!(state.window = sippetCreateWindow(state.application, "SippetBox", 640, 480)))
    return (sippetLogError("Failed to create window"), terminate(&state, 1));

  while (sippetUpdate() && !sippetWindowShouldClose(state.window)) {
    sippetDrawSomething(state.window);
    sippetUpdateWindow(state.window);
  }

  return terminate(&state, 0);
}

static int terminate(struct State *const state, const int exitCode) {
  sippetDestroyWindow(state->window);
  sippetDestroyApplication(state->application);
  sippetTerminate();
  return exitCode;
}