#pragma once
#include <stdbool.h>

/**
 * Initializes the engine by setting up platform-specific resources and other subsystems needed for
 * the engine to function.
 */
bool initializeEngine(void);

/**
 * Processes the events and updates the windows to keep them responsive. Should be called once per
 * frame. Returns `true` for success and `false` if updating the engine fails or if it should exit.
 */
bool updateEngine(void);

/**
 * Shuts down the engine by cleaning up platform-specific resources and other subsystems needed for
 * the engine to function.
 */
void terminateEngine(void);