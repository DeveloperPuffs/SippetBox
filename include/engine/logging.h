#pragma once

/**
 * Outputs an error message onto the console if in debug build. Will be ignored in release build.
 */
void log_error(const char *format, ...);

/**
 * Outputs a warning message onto the console if in debug build. Will be ignored in release build.
 */
void log_warning(const char *format, ...);

/**
 * Outputs an information message onto the console if in debug build. Will be ignored in release build.
 */
void log_information(const char *format, ...);

/**
 * Outputs a debug message onto the console if in debug build. Will be ignored in release build.
 */
void log_debug(const char *format, ...);