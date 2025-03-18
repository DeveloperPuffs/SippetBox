#pragma once

/**
 * Stores resources that can be shared between multiple renderers/windows. Contains a Vulkan
 * instance, a physical device, a logical device, and multiple queues.
 */
typedef struct Context Context;

/**
 * Creates an instance of `Context` and initializes its resources.
 */
Context *context_create();

/**
 * Destroys an instance of `Context` by freeing its resources.
 */
void context_destroy(Context *graphics_backend);

/**
 * Stores resources used by a window for creating an environment that can be renderered on. Contains an SDL window
 * handle, a window extent, a window surface, a surface format, a swapchain, the swapchain image count, swapchain
 * images, image views, a render pass, the shader module count for destroying the shader modules one by one, shader
 * modules for the vertex shader and the fragment shader, a pipeline layout, a pipeline for rendering operations,
 * framebuffers, the maximum number of frames in flight, a command pool, command buffers, synchronization objects, and
 * the current frame index for rendering multiple frames concurrently.
 */
typedef struct Window Window;

/**
 * Creates an instance of `Window` and initializes its resources. Opens an SDL window with the title as a string and
 * the initial dimensions of the window as positive integers. Is associated with the given instance of `Context`.
 */
Window *window_create(const char *title, int initial_width, int initial_height, Context *context);

/**
 * Renders a frame on a `Window` by recording a command buffer, waiting for drawing operations to be sent when calling
 * the callback function, and ending the command buffer for submission to the graphics queue, and then to the present
 * queue after the rendering operations have been executed on the GPU. Returns `1` for success and `0` for failure.
 */
int window_render_frame(Window *window, void (*callback)(void *user_data), void *user_data);

/**
 * Destroys an instance of `Window` by freeing its resources. Closes the SDL window associated with the given instance.
 */
void window_destroy(Window *window);