#include <sippet/core/platform.h>
#include <sippet/core/logging.h>

#ifdef NDEBUG
void sippetLogError(const char *format, ...) (void)format;
void sippetLogWarning(const char *format, ...) (void)format;
void sippetLogInformation(const char *format, ...) (void)format;
void sippetLogDebug(const char *format, ...) (void)format;
#else
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#ifdef SIPPET_PLATFORM_WINDOWS
#include <windows.h>

static bool sippetCheckANSISupport() {
  HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;

  if (!GetConsoleMode(outputHandle, &mode)) {
    SetConsoleMode(outputHandle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    return true;
  }

  return false;
}
#else
#include <stdlib.h>

static bool sippetCheckANSISupport() {
  const char *term = getenv("TERM");
  if (!term) return 0;

  return
    strstr(term, "screen") ||
    strstr(term, "xterm") ||
    strstr(term, "vt100") ||
    strstr(term, "linux") ||
    strstr(term, "tmux") ||
    strstr(term, "rxvt");
}
#endif

#define ANSI (sippetCheckANSISupport())

void sippetLogError(const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, ANSI ? "\033[1;31mERROR\033[22;0m:         " : "ERROR:         ");
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
  va_end(args);
}

void sippetLogWarning(const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, ANSI ? "\033[1;33mWARNING\033[22;0m:       " : "WARNING:       ");
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
  va_end(args);
}

void sippetLogInformation(const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, ANSI ? "\033[1;37mINFORMATION\033[22;0m:   " : "INFORMATION:   ");
  vfprintf(stdout, format, args);
  fprintf(stdout, "\n");
  va_end(args);
}

void sippetLogDebug(const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, ANSI ? "\033[1;34mDEBUG\033[22;0m:        " : "DEBUG:         ");
  vfprintf(stdout, format, args);
  fprintf(stdout, "\n");
  va_end(args);
}
#endif