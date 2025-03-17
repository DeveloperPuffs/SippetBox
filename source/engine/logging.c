#include <engine/platform.h>

#include <engine/logging.h>

#ifdef NDEBUG
void log_error(const char *format, ...)
{
    (void)format;
}

void log_warning(const char *format, ...)
{
    (void)format;
}

void log_information(const char *format, ...)
{
    (void)format;
}

void log_debug(const char *format, ...)
{
    (void)format;
}
#else
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef PLATFORM_WINDOWS
#include <windows.h>

int ansi() {
    HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;

    if (GetConsoleMode(output_handle, &mode) == TRUE)
    {
        SetConsoleMode(output_handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        return 1;
    }

    return 0;
}
#else
#include <stdlib.h>

int ansi() {
    const char *term = getenv("TERM");
    if (term == NULL)
    {
        return 0;
    }

    return
    (
        strstr(term, "screen") != NULL ||
        strstr(term, "xterm") != NULL ||
        strstr(term, "vt100") != NULL ||
        strstr(term, "linux") != NULL ||
        strstr(term, "tmux") != NULL ||
        strstr(term, "rxvt") != NULL
    );
}
#endif

#define ANSI (ansi())

void log_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, ANSI ? "\033[1;31mERROR\033[22;0m:         " : "ERROR:          ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void log_warning(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, ANSI ? "\033[1;33mWARNING\033[22;0m:       " : "WARNING:        ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void log_information(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, ANSI ? "\033[1;37mINFORMATION\033[22;0m:   " : "INFORMATION:    ");
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    va_end(args);
}

void log_debug(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, ANSI ? "\033[1;34mDEBUG\033[22;0m:         " : "DEBUG:          ");
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    va_end(args);
}
#endif