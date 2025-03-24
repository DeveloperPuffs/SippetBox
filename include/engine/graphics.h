#pragma once

/**
 * Stores resources that can be shared between multiple renderers/windows.
 */
struct Context;

/**
 * Creates an instance of `Context` and initializes its resources.
 */
struct Context *context_create();

/**
 * Destroys an instance of `Context` by freeing its resources.
 */
void context_destroy(struct Context *graphics_backend);

/**
 * Stores resources used by a window for creating an environment that can be renderered on.
 */
struct Window;

/**
 * Creates an instance of `Window` and initializes its resources. Opens an SDL window with the title as a string and
 * the initial dimensions of the window as positive integers. Is associated with the given instance of `Context`.
 */
struct Window *window_create
(
    const char *title, const unsigned int initial_width, const unsigned int initial_height, struct Context *context
);

/**
 * Renders a frame on a `Window` by submitting the recorded rendering commands for execution, and then for presenting
 * after the rendering operations have been executed on the GPU. Returns `1` for success and `0` for failure.
 */
unsigned char window_update(struct Window *const window);

/**
 * Renders something onto the window :)
 */
void window_render_something(struct Window *const window);

/**
 * Destroys an instance of `Window` by freeing its resources. Closes the SDL window associated with the given instance.
 */
void window_destroy(struct Window *window);