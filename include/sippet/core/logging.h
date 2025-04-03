#pragma once

/**
 * Outputs an error message onto the console if in debug build. Will be ignored in release build.
 */
void sippetLogError(const char *format, ...);

/**
 * Outputs a warning message onto the console if in debug build. Will be ignored in release build.
 */
void sippetLogWarning(const char *format, ...);

/**
 * Outputs an information message onto the console if in debug build. Will be ignored in release build.
 */
void sippetLogInformation(const char *format, ...);

/**
 * Outputs a debug message onto the console if in debug build. Will be ignored in release build.
 */
void sippetLogDebug(const char *format, ...);