#pragma once

/**
 * Initializes the mach timebase information data, used for getting the current time.
 */
void sippetAppleInitializeTime(void);

/**
 * Returns the current time in seconds since the system boot.
 */
double sippetAppleGetCurrentTime(void);