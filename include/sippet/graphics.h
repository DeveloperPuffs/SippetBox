#pragma once
#include <stdbool.h>
#include <stdint.h>

/**
 * Stores resources typically used in one application that can be used to create windows.
 */
struct SippetApplication;

/**
 * Creates an instance of `Application` and initializes its resources.
 */
struct SippetApplication *sippetCreateApplication(const char *name);

/**
 * Destroys an instance of `Application` by freeing its resources.
 */
void sippetDestroyApplication(struct SippetApplication *const application);

/**
 * Stores resources used by a window for creating an environment that can be renderered on.
 */
struct SippetWindow;

/**
 * Creates an instance of `SippetWindow` and initializes its resources. Opens an SDL window with
 * the title as a string and the initial dimensions of the window as positive integers. Is
 * associated with the given instance of `Application`.
 */
struct SippetWindow *sippetCreateWindow(const struct SippetApplication *application, const char *const title, const uint32_t width, const uint32_t height);

/**
 * Draws a frame on a `SippetWindow` by submitting the recorded rendering commands for execution,
 * and then for presenting after the rendering operations have been executed on the GPU.
 */
bool sippetUpdateWindow(struct SippetWindow *const window);

/**
 * Checks whether the given `SippetWindow` needs to close.
 */
bool sippetWindowShouldClose(struct SippetWindow *const window);

/**
 * Draws something onto the window :)
 */
void sippetDrawSomething(struct SippetWindow *const window);

/**
 * Destroys an instance of `SippetWindow` by freeing its resources. Closes the SDL window
 * associated with the given instance.
 */
void sippetDestroyWindow(struct SippetWindow *const window);