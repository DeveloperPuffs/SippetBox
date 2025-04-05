#pragma once

/**
 * Represents the severity of a message passed to the message callback.
 */
enum SippetMessageSeverity {
  SIPPET_MESSAGE_SEVERITY_NONE,
  SIPPET_MESSAGE_SEVERITY_FATAL,
  SIPPET_MESSAGE_SEVERITY_ERROR,
  SIPPET_MESSAGE_SEVERITY_WARNING,
  SIPPET_MESSAGE_SEVERITY_INFORMATION,
  SIPPET_MESSAGE_SEVERITY_NOTICE,
  SIPPET_MESSAGE_SEVERITY_DEBUG,
  SIPPET_MESSAGE_SEVERITY_VERBOSE,
  SIPPET_MESSAGE_SEVERITY_TRACE,
  SIPPET_MESSAGE_SEVERITY_MAXIMUM
};

/**
 * Converts a message severity to a string representation.
 */
static inline const char *sippetMessageSeverityString(const enum SippetMessageSeverity messageSeverity) {
  switch (messageSeverity) {
    case SIPPET_MESSAGE_SEVERITY_NONE:
      return "SIPPET_MESSAGE_SEVERITY_NONE";
    case SIPPET_MESSAGE_SEVERITY_FATAL:
      return "SIPPET_MESSAGE_SEVERITY_FATAL";
    case SIPPET_MESSAGE_SEVERITY_ERROR:
      return "SIPPET_MESSAGE_SEVERITY_ERROR";
    case SIPPET_MESSAGE_SEVERITY_WARNING:
      return "SIPPET_MESSAGE_SEVERITY_WARNING";
    case SIPPET_MESSAGE_SEVERITY_INFORMATION:
      return "SIPPET_MESSAGE_SEVERITY_INFORMATION";
    case SIPPET_MESSAGE_SEVERITY_NOTICE:
      return "SIPPET_MESSAGE_SEVERITY_NOTICE";
    case SIPPET_MESSAGE_SEVERITY_DEBUG:
      return "SIPPET_MESSAGE_SEVERITY_DEBUG";
    case SIPPET_MESSAGE_SEVERITY_VERBOSE:
      return "SIPPET_MESSAGE_SEVERITY_VERBOSE";
    case SIPPET_MESSAGE_SEVERITY_TRACE:
      return "SIPPET_MESSAGE_SEVERITY_TRACE";
    case SIPPET_MESSAGE_SEVERITY_MAXIMUM:
      return "SIPPET_MESSAGE_SEVERITY_MAXIMUM";
    default:
      return "SIPPET_MESSAGE_SEVERITY_NONE";
  }
}

/**
 * Callback function type for message handling.
 */
typedef void (*SippetMessageCallback)(const enum SippetMessageSeverity messageSeverity, const char *message, void *userData);

/**
 * Sets the message callback function.
 */
void sippetSetMessageCallback(SippetMessageCallback messageCallback, void *userData);

/**
 * Sends a message to the current message callback.
 */
void sippetSendMessage(const enum SippetMessageSeverity messageSeverity, const char *message, ...);