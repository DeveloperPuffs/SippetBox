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
 * framebuffers, a command pool, command buffers, and synchronization objects for rendering multiple frames.
 */
typedef struct Window Window;

/**
 * Creates an instance of `Window` and initializes its resources. Opens an SDL window with the title as a string and
 * the initial dimensions of the window as positive integers. Is associated with the given instance of `Context`.
 */
Window *window_create(const char *title, int initial_width, int initial_height, Context *context);

/**
 * Destroys an instance of `Window` by freeing its resources. Closes the SDL window associated with the given instance.
 */
void window_destroy(Window *window);