#pragma once

/**
 * Stores resources that can be shared between multiple renderers/windows. Contains a Vulkan
 * instance, a physical device, a (logical) device, and multiple queues.
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