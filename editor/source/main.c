#include <stdio.h>
#include <sippet/core/message.h>
#include <sippet/core/platform.h>
#include <sippet/graphics.h>
#include <sippet/sippet.h>

struct State {
  struct SippetApplication *application;
  struct SippetWindow *window;
};

static void sippetMessageCallback(enum SippetMessageSeverity messageSeverity, const char *message, void *userData) {
  (void)userData;

  FILE *const stream = messageSeverity <= SIPPET_MESSAGE_SEVERITY_ERROR ? stderr : stdout;
  const char *const severityString = sippetMessageSeverityString(messageSeverity);
  fprintf(stream, "Sippet(%s): %s\n", severityString, message);
  fflush(stream);
}

static inline int terminate(struct State *const state, const int exitCode) {
  sippetDestroyWindow(state->window);
  sippetDestroyApplication(state->application);
  sippetTerminate();
  return exitCode;
}

int main(void) {
  if (!sippetInitialize()) return 1;
  sippetSetMessageCallback(sippetMessageCallback, NULL);

  struct State state = {};
  state.application = NULL;
  state.window = NULL;

  if (!(state.application = sippetCreateApplication("SippetBox")))
    return terminate(&state, 1);

  if (!(state.window = sippetCreateWindow(state.application, "SippetBox", 640, 480)))
    return terminate(&state, 1);

  while (sippetUpdate() && !sippetWindowShouldClose(state.window)) {
    sippetDrawSomething(state.window);
    sippetUpdateWindow(state.window);
  }

  return terminate(&state, 0);
}