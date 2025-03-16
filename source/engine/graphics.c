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

static inline VkBool32 vulkan_debug_messenger_callback
(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data
);

static inline uint32_t rate_physical_device_properties(VkPhysicalDevice physical_device);
static inline uint32_t rate_physical_device_features(VkPhysicalDevice physical_device);
static inline uint32_t rate_physical_device_memory(VkPhysicalDevice physical_device);
static inline uint32_t rate_physical_device_queue_families(VkPhysicalDevice physical_device);
static inline uint32_t rate_physical_device_extensions(VkPhysicalDevice physical_device);

/*********************************************************************************************************************/
// Graphics Backend Creation Function
/*********************************************************************************************************************/

GraphicsBackend *graphics_backend_create()
{
    VkResult result;
    
    GraphicsBackend *graphics_backend = (GraphicsBackend *)calloc(1, sizeof(GraphicsBackend));
    if (graphics_backend == NULL)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory for graphics backend instance\n");
        return NULL;
    }

    /*****************************************************************************************************************/
    // Vulkan Instance Creation
    /*****************************************************************************************************************/

    VkInstanceCreateInfo instance_creation_information = {0};
    instance_creation_information.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_creation_information.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    VkApplicationInfo application_information = {0};
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

    // VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME - Enabled to be able to run Vulkan on a macOS via MoltenVK
    // VK_EXT_DEBUG_UTILS_EXTENSION_NAME - Enabled to be able to setup a debug messenger with the instance
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

    /*****************************************************************************************************************/
    // Vulkan Debug Messenger Creation
    /*****************************************************************************************************************/

    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_creation_information = {0};
    debug_messenger_creation_information.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_messenger_creation_information.pfnUserCallback = vulkan_debug_messenger_callback;
    debug_messenger_creation_information.pUserData = NULL;

    debug_messenger_creation_information.messageSeverity =
    (
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
    );

    debug_messenger_creation_information.messageType =
    (
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
    );

    PFN_vkCreateDebugUtilsMessengerEXT debug_messenger_creation_function =
    (
        (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(graphics_backend->vulkan_instance, "vkCreateDebugUtilsMessengerEXT")
    );

    if (debug_messenger_creation_function == NULL)
    {
        fprintf(stderr, "ERROR: Failed to query debug messenger creation function\n");
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    result = debug_messenger_creation_function
    (
        graphics_backend->vulkan_instance, &debug_messenger_creation_information, NULL,
        &graphics_backend->debug_messenger
    );

    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to create debug messenger: %s\n", string_VkResult(result));
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Physical Device Selection
    /*****************************************************************************************************************/

    uint32_t physical_device_count;
    result = vkEnumeratePhysicalDevices(graphics_backend->vulkan_instance, &physical_device_count, NULL);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to query physical device count: %s\n", string_VkResult(result));
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    if (physical_device_count == 0)
    {
        fprintf(stderr, "ERROR: No physical device found\n");
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    VkPhysicalDevice *physical_devices = malloc(physical_device_count * sizeof(VkPhysicalDevice));
    if (physical_devices == NULL)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory for physical devices\n");
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    result = vkEnumeratePhysicalDevices(graphics_backend->vulkan_instance, &physical_device_count, physical_devices);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to query physical devices: %s\n", string_VkResult(result));
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    uint32_t best_score = 0;
    for (uint32_t physical_device_index = 0; physical_device_index < physical_device_count; ++physical_device_index)
    {
        VkPhysicalDevice physical_device = physical_devices[physical_device_index];
        uint32_t score =
        (
            rate_physical_device_properties(physical_device) +
            rate_physical_device_features(physical_device) +
            rate_physical_device_memory(physical_device) +
            rate_physical_device_queue_families(physical_device) +
            rate_physical_device_extensions(physical_device)
        );

        if (score > best_score)
        {
            graphics_backend->physical_device = physical_device;
            best_score = score;
        }
    }

    free(physical_devices);
    if (best_score == 0)
    {
        fprintf(stderr, "ERROR: Failed to find suitable physical device\n");
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    fprintf(stdout, "INFORMATION: Selected physical device with score %d\n", best_score);
    
    /*****************************************************************************************************************/
    // Graphics Queue Family Index and Present Queue Family Index Selection
    /*****************************************************************************************************************/

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(graphics_backend->physical_device, &queue_family_count, NULL);

    VkQueueFamilyProperties *queue_families = malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    if (queue_families == NULL)
    {
        fprintf(stdout, "ERROR: Failed to allocate memory for queue families\n");
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(graphics_backend->physical_device, &queue_family_count, queue_families);

    // A window and a window surface have to be created to ensure that the present queue family supports presenting to
    // a window surface created in this context
    SDL_Window *window_model = SDL_CreateWindow("", 1, 1, SDL_WINDOW_VULKAN);
    if (window_model == NULL)
    {
        fprintf(stderr, "ERROR: Failed to create window model: %s\n", SDL_GetError());
        graphics_backend_destroy(graphics_backend);
        free(queue_families);
        return NULL;
    }

    VkSurfaceKHR surface_model;
    if (SDL_Vulkan_CreateSurface(window_model, graphics_backend->vulkan_instance, NULL, &surface_model) == false)
    {
        fprintf(stderr, "ERROR: Failed to create surface model: %s\n", SDL_GetError());
        graphics_backend_destroy(graphics_backend);
        SDL_DestroyWindow(window_model);
        free(queue_families);
        return NULL;
    }

    uint32_t graphics_queue_family_index = UINT32_MAX;
    uint32_t present_queue_family_index = UINT32_MAX;
    for (uint32_t queue_family_index = 0; queue_family_index < queue_family_count; ++queue_family_index)
    {
        if (queue_families[queue_family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            if (graphics_queue_family_index == UINT32_MAX)
            {
                graphics_queue_family_index = queue_family_index;
            }
        }

        VkBool32 supports_presenting = VK_FALSE;
        result = vkGetPhysicalDeviceSurfaceSupportKHR
        (
            graphics_backend->physical_device, queue_family_index, surface_model, &supports_presenting
        );

        if (result != VK_SUCCESS)
        {
            fprintf
            (
                stderr,
                "WARNING: Failed to query physical device surface queue family #%d presenting support verdict: %s",
                queue_family_index, string_VkResult(result)
            );

            continue;
        }

        if (present_queue_family_index == UINT32_MAX && supports_presenting)
        {
            present_queue_family_index = queue_family_index;
        }
    }

    vkDestroySurfaceKHR(graphics_backend->vulkan_instance, surface_model, NULL);
    SDL_DestroyWindow(window_model);
    free(queue_families);

    if (graphics_queue_family_index == UINT32_MAX || present_queue_family_index == UINT32_MAX)
    {
        fprintf(stderr, "ERROR: Failed to find suitable queue family indices\n");
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    fprintf
    (
        stdout, "INFORMATION: Using physical device queue family index %d for graphics queue family\n",
        graphics_queue_family_index
    );

    fprintf
    (
        stdout, "INFORMATION: Using physical device queue family index %d for present queue family\n",
        present_queue_family_index
    );

    /*****************************************************************************************************************/
    // Logical Device Creation
    /*****************************************************************************************************************/

    VkDeviceCreateInfo device_creation_information = {};
    device_creation_information.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    float queue_priority = 1.0f;
    uint32_t queue_creation_information_count = 1;

    VkDeviceQueueCreateInfo queue_creation_informations[2] = {};
    queue_creation_informations[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_creation_informations[0].queueFamilyIndex = graphics_queue_family_index;
    queue_creation_informations[0].pQueuePriorities = &queue_priority;
    queue_creation_informations[0].queueCount = 1;

    if (present_queue_family_index != graphics_queue_family_index)
    {
        queue_creation_information_count++;
        queue_creation_informations[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_creation_informations[1].queueFamilyIndex = present_queue_family_index;
        queue_creation_informations[1].pQueuePriorities = &queue_priority;
        queue_creation_informations[1].queueCount = 1;
    }

    device_creation_information.pQueueCreateInfos = queue_creation_informations;
    device_creation_information.queueCreateInfoCount = queue_creation_information_count;

    uint32_t logical_device_extension_count = 0;
    result = vkEnumerateDeviceExtensionProperties
    (
        graphics_backend->physical_device, NULL, &logical_device_extension_count, NULL
    );

    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to query logical device extension count: %s\n", string_VkResult(result));
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    VkExtensionProperties *logical_device_extensions = malloc
    (
        logical_device_extension_count * sizeof(VkExtensionProperties)
    );

    if (logical_device_extensions == NULL)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory for logical device extensions\n");
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    result = vkEnumerateDeviceExtensionProperties
    (
        graphics_backend->physical_device, NULL, &logical_device_extension_count, logical_device_extensions
    );
  
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to query logical device extensions: %s\n", string_VkResult(result));
        graphics_backend_destroy(graphics_backend);
        free(logical_device_extensions);
        return NULL;
    }

    // TODO: Enable the following logical device extensions for ray tracing or path tracing
    // VK_KHR_acceleration_structure, VK_KHR_ray_tracing_pipeline, VK_KHR_deferred_host_operations
    
    // VK_KHR_SWAPCHAIN_EXTENSION_NAME - Enabled to be able present on a window surface
    // VK_KHR_portability_subset - Enabled to be able to run Vulkan on a macOS via MoltenVK
    const char *enabled_logical_device_extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
    };

    uint32_t enabled_logical_device_extension_count =
    (
        sizeof(enabled_logical_device_extensions) / sizeof(enabled_logical_device_extensions[0])
    );

    for
    (
        uint32_t enabled_logical_device_extension_index = 0;
        enabled_logical_device_extension_index < enabled_logical_device_extension_count;
        ++enabled_logical_device_extension_index
    )
    {
        VkBool32 logical_device_extension_found = VK_FALSE;
        const char *enabled_logical_device_extension_name =
        (
            enabled_logical_device_extensions[enabled_logical_device_extension_index]
        );

        for
        (
            uint32_t logical_device_extension_index = 0;
            logical_device_extension_index < logical_device_extension_count;
            ++logical_device_extension_index
        )
        {
            const char *logical_device_extension_name =
            (
                logical_device_extensions[logical_device_extension_index].extensionName
            );

            if (strcmp(enabled_logical_device_extension_name, logical_device_extension_name) == 0)
            {
                logical_device_extension_found = VK_TRUE;
                break;
            }
        }

        if (logical_device_extension_found == VK_FALSE)
        {
            fprintf
            (
                stderr, "ERROR: Logical device extension '%s' requested, but not found\n",
                enabled_logical_device_extension_name
            );

            graphics_backend_destroy(graphics_backend);
            free(logical_device_extensions);
            return NULL;
        }
    }
    
    free(logical_device_extensions);
    device_creation_information.enabledExtensionCount = enabled_logical_device_extension_count;
    device_creation_information.ppEnabledExtensionNames = enabled_logical_device_extensions;

    const char *logical_device_validation_layers[] =
    {
        "VK_LAYER_KHRONOS_validation"
    };

    device_creation_information.enabledLayerCount = 1;
    device_creation_information.ppEnabledLayerNames = logical_device_validation_layers;

    result = vkCreateDevice
    (
        graphics_backend->physical_device, &device_creation_information, NULL, &graphics_backend->logical_device
    );

    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "ERROR: Failed to create logical device: %s\n", string_VkResult(result));
        graphics_backend_destroy(graphics_backend);
        return NULL;
    }

    return graphics_backend;
}

/*********************************************************************************************************************/
// Graphics Backend Destruction Function
/*********************************************************************************************************************/

void graphics_backend_destroy(GraphicsBackend *graphics_backend)
{
    if (graphics_backend == NULL)
    {
        return;
    }

    vkDestroyDevice(graphics_backend->logical_device, NULL);

    if (graphics_backend->vulkan_instance != NULL)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT debug_messenger_destruction_function =
        (
            (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(graphics_backend->vulkan_instance, "vkDestroyDebugUtilsMessengerEXT")
        );

        if (debug_messenger_destruction_function)
        {
            debug_messenger_destruction_function
            (
                graphics_backend->vulkan_instance, graphics_backend->debug_messenger, NULL
            );
        }
        else
        {
            fprintf(stderr, "WARNING: Failed to query debug messenger destruction function\n");
        }
    }

    vkDestroyInstance(graphics_backend->vulkan_instance, NULL);
    free(graphics_backend);
}

/*********************************************************************************************************************/
// Vulkan Debug Messenger Callback Function
/*********************************************************************************************************************/

static inline VkBool32 vulkan_debug_messenger_callback
(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity_flags, VkDebugUtilsMessageTypeFlagsEXT message_type_flags,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data
)
{
    char type_string[64] = {0};
    VkBool32 leading_comma = VK_FALSE;

    if (message_type_flags & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
    {
        snprintf(type_string, sizeof(type_string), "GENERAL");
        leading_comma = VK_TRUE;
    }

    if (message_type_flags & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
    {
        if (leading_comma)
        {
            size_t length = strlen(type_string);
            snprintf(type_string + length, sizeof(type_string) - length, ", VALIDATION");
        }
        else
        {
            snprintf(type_string, sizeof(type_string), "VALIDATION");
            leading_comma = VK_TRUE;
        }
    }

    if (message_type_flags & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
    {
        if (leading_comma)
        {
            size_t length = strlen(type_string);
            snprintf(type_string + length, sizeof(type_string) - length, ", PERFORMANCE");
        }
        else
        {
            snprintf(type_string, sizeof(type_string), "PERFORMANCE");
            leading_comma = VK_TRUE;
        }
    }

    if (message_type_flags & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
    {
        if (leading_comma)
        {
            size_t length = strlen(type_string);
            snprintf(type_string + length, sizeof(type_string) - length, ", DEVIVE_ADDRESS_BINDING");
        }
        else
        {
            snprintf(type_string, sizeof(type_string), "DEVIVE_ADDRESS_BINDING");
            leading_comma = VK_TRUE;
        }
    }

    if (message_severity_flags & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        fprintf(stderr, "ERROR (%s): %s\n", type_string, callback_data->pMessage);
    }

    if (message_severity_flags & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        fprintf(stderr, "WARNING (%s): %s\n", type_string, callback_data->pMessage);
    }

    if (message_severity_flags & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        fprintf(stdout, "INFORMATION (%s): %s\n", type_string, callback_data->pMessage);
    }

    if (message_severity_flags & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        fprintf(stdout, "DEBUG (%s): %s\n", type_string, callback_data->pMessage);
    }

    return VK_FALSE;
}

/*********************************************************************************************************************/
// Physical Device Criteria
/*********************************************************************************************************************/

static inline uint32_t rate_physical_device_properties(VkPhysicalDevice physical_device)
{
    uint32_t score = 0;

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(physical_device, &device_properties);

    // It would be nice to have the physical device support Vulkan version 1.3 at least
    uint32_t api_version = device_properties.apiVersion;
    uint32_t major_version = VK_VERSION_MAJOR(api_version);
    uint32_t minor_version = VK_VERSION_MINOR(api_version);
    if (major_version >= 2 || (major_version == 1 && minor_version >= 3))
    {
        score += 500;
    }

    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }

    VkPhysicalDeviceLimits limits = device_properties.limits;
    score += limits.maxImageDimension2D;

    return score;
}

static inline uint32_t rate_physical_device_features(VkPhysicalDevice physical_device)
{
    uint32_t score = 0;

    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(physical_device, &device_features);

    if (device_features.samplerAnisotropy == VK_TRUE)
    {
        score += 500;
    }

    if (device_features.geometryShader == VK_TRUE)
    {
        score += 200;
    }

    return score;
}

static inline uint32_t rate_physical_device_memory(VkPhysicalDevice physical_device)
{
    uint32_t score = 0;

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    uint32_t total_memory = 0;
    for (uint32_t memory_heap_index = 0; memory_heap_index < memory_properties.memoryHeapCount; memory_heap_index++)
    {
        VkMemoryHeap memory_heap = memory_properties.memoryHeaps[memory_heap_index];
        if (memory_heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            // The total memory will be converted to megabytes
            total_memory += memory_heap.size / (1024 * 1024);
        }
    }

    return total_memory / 10;
}

static inline uint32_t rate_physical_device_queue_families(VkPhysicalDevice physical_device)
{
    uint32_t score = 0;

    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);

    VkQueueFamilyProperties *queue_families = malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    if (queue_families == NULL)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory for physical device queue families\n");
        return score;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);
    for (uint32_t queue_family_index = 0; queue_family_index < queue_family_count; ++queue_family_index)
    {
        VkQueueFlags queue_family_flags = queue_families[queue_family_index].queueFlags;

        if (queue_family_flags & VK_QUEUE_GRAPHICS_BIT)
        {
            score += 500;
        }

        if (queue_family_flags & VK_QUEUE_COMPUTE_BIT)
        {
            score += 300;
        }
        
        if (queue_family_flags & VK_QUEUE_TRANSFER_BIT)
        {
            score += 200;
        }
    }

    free(queue_families);
    return score;
}

static inline uint32_t rate_physical_device_extensions(VkPhysicalDevice physical_device)
{
    VkResult result;
    uint32_t score = 0;

    uint32_t extension_count;
    result = vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, NULL);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "WARNING: Failed to query physical device extension count: %s\n", string_VkResult(result));
        return score;
    }

    VkExtensionProperties *extensions = malloc(extension_count * sizeof(VkExtensionProperties));
    if (extensions == NULL)
    {
        fprintf(stderr, "ERROR: Failed to allocate memory for physical device extensions\n");
        return score;
    }
    
    result = vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, extensions);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "WARNING: Failed to query physical device extensions: %s\n", string_VkResult(result));
        free(extensions);
        return score;
    }
    
    for (uint32_t extension_index = 0; extension_index < extension_count; ++extension_index)
    {
        const char *extension_name = extensions[extension_index].extensionName;
        if (strcmp(extension_name, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
        {
            score += 1000;
            break;
        }
    }

    free(extensions);
    return score;
}