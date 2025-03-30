#pragma once
#include <stdbool.h>
#include <stdint.h>

/**
 * Stores resources typically used in one application that can be used to create windows.
 */
struct Application;

/**
 * Creates an instance of `Application` and initializes its resources.
 */
struct Application *createApplication(const char *name);

/**
 * Destroys an instance of `Application` by freeing its resources.
 */
void destroyApplication(struct Application *const application);

/**
 * Stores resources used by a window for creating an environment that can be renderered on.
 */
struct Window;

/**
 * Creates an instance of `Window` and initializes its resources. Opens an SDL window with the
 * title as a string and the initial dimensions of the window as positive integers. Is associated
 * with the given instance of `Application`.
 */
struct Window *createWindow(const struct Application *application, const char *const title, const uint32_t width, const uint32_t height);

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