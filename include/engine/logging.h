#pragma once

/**
 * Outputs an error message onto the console if in debug build. Will be ignored in release build.
 */
void logError(const char *format, ...);

/**
 * Outputs a warning message onto the console if in debug build. Will be ignored in release build.
 */
void logWarning(const char *format, ...);

/**
 * Outputs an information message onto the console if in debug build. Will be ignored in release build.
 */
void logInformation(const char *format, ...);

/**
 * Outputs a debug message onto the console if in debug build. Will be ignored in release build.
 */
void logDebug(const char *format, ...);