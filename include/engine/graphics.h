#pragma once
#include <stdbool.h>
#include <stdint.h>

/**
 * Stores resources that can be shared between multiple renderers/windows.
 */
struct Context;

/**
 * Creates an instance of `Context` and initializes its resources.
 */
struct Context *createContext();

/**
 * Destroys an instance of `Context` by freeing its resources.
 */
void destroyContext(struct Context *const context);

/**
 * Stores resources used by a window for creating an environment that can be renderered on.
 */
struct Window;

/**
 * Creates an instance of `Window` and initializes its resources. Opens an SDL window with the
 * title as a string and the initial dimensions of the window as positive integers. Is associated
 * with the given instance of `Context`.
 */
struct Window *createWindow(const char *const title, const uint32_t width, const uint32_t height, const struct Context *context);

/**
 * Draws a frame on a `Window` by submitting the recorded rendering commands for execution, and
 * then for presenting after the rendering operations have been executed on the GPU. Returns `true`
 * for success and `false` for failure.
 */
bool updateWindow(struct Window *const window);

/**
 * Draws something onto the window :)
 */
void drawSomething(struct Window *const window);

/**
 * Destroys an instance of `Window` by freeing its resources. Closes the SDL window associated with
 * the given instance.
 */
void destroyWindow(struct Window *const window);