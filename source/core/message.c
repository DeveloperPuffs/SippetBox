#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include <sippet/core/message.h>

static SippetMessageCallback sippetCurrentMessageCallback = NULL;
static void *sippetCurrentMessageCallbackUserData = NULL;

void sippetSetMessageCallback(SippetMessageCallback messageCallback, void *userData) {
  sippetCurrentMessageCallback = messageCallback;
  sippetCurrentMessageCallbackUserData = userData;
}

void sippetSendMessage(const enum SippetMessageSeverity messageSeverity, const char *message, ...) {
  if (!sippetCurrentMessageCallback) return;

  va_list arguments;
  va_start(arguments, message);

  const int size = vsnprintf(NULL, 0, message, arguments) + 1;
  char *formattedMessage = malloc(size);
  if (!formattedMessage) {
    va_end(arguments);
    return;
  }

  vsnprintf(formattedMessage, size, message, arguments);
  sippetCurrentMessageCallback(messageSeverity, formattedMessage, sippetCurrentMessageCallbackUserData);
  free(formattedMessage);
  va_end(arguments);
}