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
 * handle, a window surface, a swapchain, swapchain images, image views, framebuffers, and synchronization objects for
 * rendering multiple frames.
 */
typedef struct Window Window;

/**
 * Creates an instance of `Window` and initializes its resources. Opens an SDL window with the title as a string and
 * the initial dimensions of the window as positive integers. 
 */
Window *window_create(const char *title, int initial_width, int initial_height);

/**
 * Destroys an instance of `Window` by freeing its resources. Closes the SDL window associated with the given instance.
 */
void window_destroy(Window *window);

/**
 * Stores resources used for rendering on a specific environment (for example a 'Window' instance). Contains a
 * pipeline, a render pass, a command pool, and command buffers.
 */
typedef struct Renderer Renderer;

/**
 * Creates an instance of `Renderer` and initializes its resources.
 */
Renderer *renderer_create();

/**
 * Destroys an instance of `Renderer` by freeing its resources.
 */
void renderer_destroy(Renderer *renderer);