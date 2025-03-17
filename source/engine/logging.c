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

void log_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void log_warning(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stderr, "WARNING: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void log_information(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stdout, "INFORMATION: ");
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    va_end(args);
}

void log_debug(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    fprintf(stdout, "DEBUG: ");
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    va_end(args);
}
#endif