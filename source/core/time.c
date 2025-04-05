#include "../platform/apple/time.h"
#include <sippet/core/platform.h>
#include <sippet/core/message.h>
#include <sippet/core/time.h>

double sippetGetCurrentTime(void) {
#ifdef SIPPET_PLATFORM_APPLE
  return sippetAppleGetCurrentTime();
#else
  sippetSendMessage(SIPPET_MESSAGE_SEVERITY_WARNING, "Current time retrieval not implemented for this platform");
  return 0.0f;
#endif
}