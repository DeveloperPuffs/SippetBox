#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL_vulkan.h>
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
    VkResult result;

    GraphicsBackend *graphics_backend = (GraphicsBackend *)calloc(1, sizeof(GraphicsBackend));
    if (graphics_backend == NULL)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory for graphics backend instance");
        return NULL;
    }

    VkInstanceCreateInfo instance_creation_information = {};
    instance_creation_information.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_creation_information.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    
    VkApplicationInfo application_information = {};
    application_information.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_information.pApplicationName = "SippetBox";
    application_information.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    application_information.pEngineName = "No Engine";
    application_information.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    application_information.apiVersion = VK_API_VERSION_1_0;
    instance_creation_information.pApplicationInfo = &application_information;

    uint32_t layer_count;
    result = vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to query layer count: %s\n", string_VkResult(result));
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    VkLayerProperties *layers = malloc(sizeof(VkLayerProperties) * layer_count);
    if (layers == NULL)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory for layers\n");
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    result = vkEnumerateInstanceLayerProperties(&layer_count, layers);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to query layers: %s\n", string_VkResult(result));
        graphics_backend_destroy(graphics_backend);
        free(layers);
        return NULL;
    }

    // An array of all of the requested layers, it should be fine if I just hardcode this for now
    const char *requested_layers[] =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    size_t requested_layers_count = sizeof(requested_layers) / sizeof(requested_layers[0]);
    for (uint32_t requested_layer_index = 0; requested_layer_index < requested_layers_count; ++requested_layer_index)
    {
        VkBool32 layer_found = VK_FALSE;
        const char *requested_layer = requested_layers[requested_layer_index];
        for (uint32_t layer_index = 0; layer_index < layer_count; ++layer_index)
        {
            if (strcmp(requested_layer, layers[layer_index].layerName) == 0)
            {
                layer_found = VK_TRUE;
                break;
            }
        }

        if (layer_found == VK_FALSE)
        {
            fprintf(stderr, "ERROR: Layer '%s' requested, but not found\n", requested_layer);
            graphics_backend_destroy(graphics_backend);
            free(layers);
            return NULL;
        }
    }

    // The layers can be freed after looking through the layers to make sure the requested layers are present
    free(layers);

    instance_creation_information.enabledLayerCount = requested_layers_count;
    instance_creation_information.ppEnabledLayerNames = requested_layers;

    uint32_t sdl_extension_count = 0;
    const char *const *sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);
    if (sdl_extensions == NULL)
    {
        fprintf(stderr, "ERROR: Failed to query SDL Vulkan instance extensions: %s\n", SDL_GetError());
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }
    
    // An array of the required (not SDL) extensions, it should be fine if I just hardcode this for now
    const char *required_extensions[] =
    {
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    size_t required_extension_count = sizeof(required_extensions) / sizeof(required_extensions[0]);
    
    uint32_t extension_count = sdl_extension_count + required_extension_count;
    const char **extensions = malloc(extension_count * sizeof(const char *));
    if (extensions == NULL)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory for extensions\n");
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    for (uint32_t extension_index = 0; extension_index < sdl_extension_count; ++extension_index)
    {
        extensions[extension_index] = sdl_extensions[extension_index];
    }

    for (uint32_t extension_index = 0; extension_index < required_extension_count; ++extension_index)
    {
        extensions[sdl_extension_count + extension_index] = required_extensions[extension_index];
    }

    instance_creation_information.enabledExtensionCount = extension_count;
    instance_creation_information.ppEnabledExtensionNames = extensions;

    result = vkCreateInstance(&instance_creation_information, NULL, &graphics_backend->vulkan_instance);

    // The extensions can be freed after creating the Vulkan instance, independent of the instance creation result
    free(extensions);

    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to create Vulkan instance: %s\n", string_VkResult(result));
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    return graphics_backend;
}

void graphics_backend_destroy(GraphicsBackend *graphics_backend)
{
    vkDestroyInstance(graphics_backend->vulkan_instance, NULL);
    free(graphics_backend);
}