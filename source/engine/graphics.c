#include <assert.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <engine/graphics.h>

typedef struct GraphicsBackend
{
    VkInstance vulkan_instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice physical_device;
    uint32_t graphics_queue_family_index;
    uint32_t present_queue_family_index;
    VkDevice logical_device;
    VkQueue graphics_queue;
    VkQueue present_queue;
} GraphicsBackend;

GraphicsBackend *graphics_backend_create()
{
    GraphicsBackend *graphics_backend = (GraphicsBackend *)malloc(sizeof(GraphicsBackend));
    if (graphics_backend == NULL)
    {
        return NULL;
    }

    return graphics_backend;
}

void graphics_backend_destroy(GraphicsBackend *graphics_backend)
{
    free(graphics_backend);
}