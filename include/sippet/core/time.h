#pragma once

/**
 * Initializes the time module, setting up the necessary data structures and
 * configurations for time management.
 */
void sippetInitializeTime(void);

/**
 * Returns the current time in seconds since the system boot.
 */
double sippetGetCurrentTime(void);