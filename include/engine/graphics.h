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
struct Window *window_create(const char *title, int initial_width, int initial_height, struct Context *context);

/**
 * Renders a frame on a `Window` by recording a command buffer, waiting for drawing operations to be sent when calling
 * the callback function, and ending the command buffer for submission to the graphics queue, and then to the present
 * queue after the rendering operations have been executed on the GPU. Returns `1` for success and `0` for failure.
 */
int window_render_frame(struct Window *window, void (*callback)(void *user_data), void *user_data);

/**
 * Renders something onto the window :)
 */
void window_render_something(struct Window *window);

/**
 * Destroys an instance of `Window` by freeing its resources. Closes the SDL window associated with the given instance.
 */
void window_destroy(struct Window *window);