#pragma once

/**
 * Stores resources that can be shared between multiple renderers/windows. Contains a Vulkan
 * instance, a physical device, a (logical) device, and multiple queues.
 */
typedef struct GraphicsBackend GraphicsBackend;

/**
 * Creates an instance of `GraphicsBackend` along with its resources.
 */
GraphicsBackend *graphics_backend_create();

/**
 * Destroys an instance of `GraphicsBackend` by freeing its resources.
 */
void graphics_backend_destroy(GraphicsBackend *graphics_backend);