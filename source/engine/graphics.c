#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <engine/platform.h>
#include <engine/logging.h>

#include <engine/graphics.h>

/*********************************************************************************************************************/
// Context and Window Definitions
/*********************************************************************************************************************/

typedef struct Context
{
    VkInstance vulkan_instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice physical_device;
    uint32_t graphics_queue_family_index;
    uint32_t present_queue_family_index;
    VkDevice logical_device;
    VkQueue graphics_queue;
    VkQueue present_queue;
} Context;

typedef struct Window
{
    Context *context;
    SDL_Window *sdl_window_handle;
    VkExtent2D window_extent;
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR surface_format;
    VkSwapchainKHR swapchain;
    uint32_t image_count;
    VkImage *images;
    VkImageView *image_views;
    VkRenderPass render_pass;
    size_t shader_module_count;
    VkShaderModule *shader_modules;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkFramebuffer *framebuffers;
    VkCommandPool command_pool;
    VkCommandBuffer *command_buffers;
    VkSemaphore *semaphores;
    VkFence *fences;
} Window;

/*********************************************************************************************************************/
// Context Functions
/*********************************************************************************************************************/

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
// Context Creation Function
/*********************************************************************************************************************/

Context *context_create()
{
    VkResult result;
    
    Context *context = (Context *)calloc(1, sizeof(Context));
    if (context == NULL)
    {
        log_error("Failed to allocate memory for context instance");
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

#ifdef NDEBUG
    const char *instance_layers[] = {};
#else
    /*
        "VK_LAYER_KHRONOS_validation"
        For reporting error messages about potential API misuse and, performance issues, or invalid synchronization
    */
    const char *instance_layers[] =
    {
        "VK_LAYER_KHRONOS_validation"
    };
#endif

    size_t instance_layer_count = sizeof(instance_layers) / sizeof(instance_layers[0]);
    instance_creation_information.enabledLayerCount = instance_layer_count;
    instance_creation_information.ppEnabledLayerNames = instance_layers;

    uint32_t sdl_instance_extension_count = 0;
    const char *const *sdl_instance_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_instance_extension_count);
    if (sdl_instance_extensions == NULL)
    {
        log_error("Failed to query SDL Vulkan instance extensions: %s", SDL_GetError());
        context_destroy(context);
        return NULL;
    }

#ifdef PLATFORM_APPLE
    /*
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        For running Vulkan on a macOS environment via MoltenVK

        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        For setting up a debug messenger associated with the Vulkan instance
    */
    const char *additional_instance_extensions[] =
    {
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
#else
    /*
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        For setting up a debug messenger associated with the Vulkan instance
    */
    const char *additional_instance_extensions[] =
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
#endif

    size_t additional_instance_extension_count =
    (
        sizeof(additional_instance_extensions) / sizeof(additional_instance_extensions[0])
    );

    size_t instance_extension_count = sdl_instance_extension_count + additional_instance_extension_count;
    const char **instance_extensions = malloc(instance_extension_count * sizeof(const char *));
    if (instance_extensions == NULL)
    {
        log_error("Failed to allocate memory for extensions");
        context_destroy(context);
        return NULL;
    }

    memcpy
    (
        instance_extensions, sdl_instance_extensions, sdl_instance_extension_count * sizeof(const char *)
    );
    
    memcpy
    (
        instance_extensions + sdl_instance_extension_count, additional_instance_extensions,
        additional_instance_extension_count * sizeof(const char *)
    );

    instance_creation_information.enabledExtensionCount = instance_extension_count;
    instance_creation_information.ppEnabledExtensionNames = instance_extensions;

    result = vkCreateInstance(&instance_creation_information, NULL, &context->vulkan_instance);

    // The instance extensions can be freed after creating the Vulkan instance, independent of the Vulkan instance
    // creation result
    free(instance_extensions);

    if (result != VK_SUCCESS)
    {
        log_error("Failed to create Vulkan instance: %s", string_VkResult(result));
        context_destroy(context);
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
        vkGetInstanceProcAddr(context->vulkan_instance, "vkCreateDebugUtilsMessengerEXT")
    );

    if (debug_messenger_creation_function == NULL)
    {
        log_error("Failed to query debug messenger creation function");
        context_destroy(context);
        return NULL;
    }

    result = debug_messenger_creation_function
    (
        context->vulkan_instance, &debug_messenger_creation_information, NULL, &context->debug_messenger
    );

    if (result != VK_SUCCESS)
    {
        log_error("Failed to create debug messenger: %s", string_VkResult(result));
        context_destroy(context);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Physical Device Selection
    /*****************************************************************************************************************/

    uint32_t physical_device_count;
    result = vkEnumeratePhysicalDevices(context->vulkan_instance, &physical_device_count, NULL);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to query physical device count: %s", string_VkResult(result));
        context_destroy(context);
        return NULL;
    }

    if (physical_device_count == 0)
    {
        log_error("No physical device found");
        context_destroy(context);
        return NULL;
    }

    VkPhysicalDevice *physical_devices = malloc(physical_device_count * sizeof(VkPhysicalDevice));
    if (physical_devices == NULL)
    {
        log_error("Failed to allocate memory for physical devices");
        context_destroy(context);
        return NULL;
    }

    result = vkEnumeratePhysicalDevices(context->vulkan_instance, &physical_device_count, physical_devices);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to query physical devices: %s", string_VkResult(result));
        context_destroy(context);
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
            context->physical_device = physical_device;
            best_score = score;
        }
    }

    free(physical_devices);
    if (best_score == 0)
    {
        log_error("Failed to find suitable physical device");
        context_destroy(context);
        return NULL;
    }

    log_information("Selected physical device with score %d", best_score);
    
    /*****************************************************************************************************************/
    // Graphics Queue Family Index and Present Queue Family Index Selection
    /*****************************************************************************************************************/

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device, &queue_family_count, NULL);

    VkQueueFamilyProperties *queue_families = malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
    if (queue_families == NULL)
    {
        log_error("Failed to allocate memory for queue families");
        context_destroy(context);
        return NULL;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device, &queue_family_count, queue_families);

    // A window and a window surface have to be created to ensure that the present queue family supports presenting to
    // a window surface created in this context
    SDL_Window *window_model = SDL_CreateWindow("", 1, 1, SDL_WINDOW_VULKAN);
    if (window_model == NULL)
    {
        log_error("Failed to create window model: %s", SDL_GetError());
        context_destroy(context);
        free(queue_families);
        return NULL;
    }

    VkSurfaceKHR surface_model;
    if (SDL_Vulkan_CreateSurface(window_model, context->vulkan_instance, NULL, &surface_model) == false)
    {
        log_error("Failed to create surface model: %s", SDL_GetError());
        context_destroy(context);
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
            context->physical_device, queue_family_index, surface_model, &supports_presenting
        );

        if (result != VK_SUCCESS)
        {
            log_warning
            (
                "Failed to query physical device surface queue family #%d presenting support verdict: %s",
                queue_family_index, string_VkResult(result)
            );

            continue;
        }

        if (present_queue_family_index == UINT32_MAX && supports_presenting)
        {
            present_queue_family_index = queue_family_index;
        }
    }

    vkDestroySurfaceKHR(context->vulkan_instance, surface_model, NULL);
    SDL_DestroyWindow(window_model);
    free(queue_families);

    if (graphics_queue_family_index == UINT32_MAX || present_queue_family_index == UINT32_MAX)
    {
        log_error("Failed to find suitable queue family indices");
        context_destroy(context);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Logical Device Creation
    /*****************************************************************************************************************/

    VkDeviceCreateInfo device_creation_information = {};
    device_creation_information.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_creation_information.pNext = NULL;

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

#ifdef NDEBUG
    const char *logical_device_layers[] = {};
#else
    /*
        "VK_LAYER_KHRONOS_validation"
        For reporting error messages about potential API misuse and, performance issues, or invalid synchronization
    */
    const char *logical_device_layers[] =
    {
        "VK_LAYER_KHRONOS_validation"
    };
#endif

    size_t logical_device_layer_count = sizeof(logical_device_layers) / sizeof(logical_device_layers[0]);

    device_creation_information.enabledLayerCount = logical_device_layer_count;
    device_creation_information.ppEnabledLayerNames = logical_device_layers;

    // TODO: Enable the following logical device extensions for ray tracing or path tracing:
    // VK_KHR_acceleration_structure, VK_KHR_ray_tracing_pipeline, VK_KHR_deferred_host_operations

#ifdef PLATFORM_APPLE
    /*
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
        or presenting on a window surface

        "VK_KHR_portability_subset"
        For running Vulkan on a macOS environment via MoltenVK
    */
    const char *logical_device_extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
    };
#else
    /*
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
        or presenting on a window surface
    */
    const char *logical_device_extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
#endif

    size_t logical_device_extension_count = sizeof(logical_device_extensions) / sizeof(logical_device_extensions[0]);

    device_creation_information.enabledExtensionCount = logical_device_extension_count;
    device_creation_information.ppEnabledExtensionNames = logical_device_extensions;

    result = vkCreateDevice
    (
        context->physical_device, &device_creation_information, NULL, &context->logical_device
    );

    if (result != VK_SUCCESS)
    {
        log_error("Failed to create logical device: %s", string_VkResult(result));
        context_destroy(context);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Fetching Graphics Queue and Present Queue
    /*****************************************************************************************************************/

    vkGetDeviceQueue
    (
        context->logical_device, context->graphics_queue_family_index, 0, &context->graphics_queue
    );

    vkGetDeviceQueue
    (
        context->logical_device, context->present_queue_family_index, 0, &context->present_queue
    );

    return context;
}

/*********************************************************************************************************************/
// Context Destruction Function
/*********************************************************************************************************************/

void context_destroy(Context *context)
{
    if (context == NULL)
    {
        return;
    }

    vkDestroyDevice(context->logical_device, NULL);

    if (context->vulkan_instance != NULL)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT debug_messenger_destruction_function =
        (
            (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(context->vulkan_instance, "vkDestroyDebugUtilsMessengerEXT")
        );

        if (debug_messenger_destruction_function)
        {
            debug_messenger_destruction_function
            (
                context->vulkan_instance, context->debug_messenger, NULL
            );
        }
        else
        {
            log_warning("Failed to query debug messenger destruction function");
        }
    }

    vkDestroyInstance(context->vulkan_instance, NULL);
    free(context);
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
        log_error("%s - %s", type_string, callback_data->pMessage);
    }

    if (message_severity_flags & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        log_warning("%s - %s", type_string, callback_data->pMessage);
    }

    if (message_severity_flags & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        log_information("%s - %s", type_string, callback_data->pMessage);
    }

    if (message_severity_flags & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        log_debug("%s - %s", type_string, callback_data->pMessage);
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
        log_error("Failed to allocate memory for physical device queue families");
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
        log_warning("Failed to query physical device extension count: %s", string_VkResult(result));
        return score;
    }

    VkExtensionProperties *extensions = malloc(extension_count * sizeof(VkExtensionProperties));
    if (extensions == NULL)
    {
        log_error("Failed to allocate memory for physical device extensions");
        return score;
    }
    
    result = vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, extensions);
    if (result != VK_SUCCESS)
    {
        log_warning("Failed to query physical device extensions: %s", string_VkResult(result));
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

/*********************************************************************************************************************/
// Window Creation Function
/*********************************************************************************************************************/

Window *window_create(const char *title, int initial_width, int initial_height, Context *context)
{
    VkResult result;

    if (context == NULL)
    {
        log_error("Cannot create 'Window' if given 'Context' is 'NULL'");
        return NULL;
    }

    Window *window = (Window *)calloc(1, sizeof(Window));
    if (window == NULL)
    {
        log_error("Failed to allocate memory for window instance");
        return NULL;
    }

    window->context = context;
    VkInstance vulkan_instance = context->vulkan_instance;
    VkPhysicalDevice physical_device = context->physical_device;
    VkDevice logical_device = context->logical_device;

    /*****************************************************************************************************************/
    // SDL Window Creation
    /*****************************************************************************************************************/

    window->sdl_window_handle = SDL_CreateWindow(title, initial_width, initial_height, SDL_WINDOW_VULKAN);
    if (window->sdl_window_handle == NULL)
    {
        log_error("Failed to create SDL window: %s", SDL_GetError());
        window_destroy(window);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Fetching Window Extent
    /*****************************************************************************************************************/

    int window_width;
    int window_height;
    if (SDL_GetWindowSizeInPixels(window->sdl_window_handle, &window_width, &window_height) == false)
    {
        log_error("Failed to query SDL window size in pixels: %s", SDL_GetError());
        window_destroy(window);
        return NULL;
    }

    window->window_extent.width = (uint32_t)window_width;
    window->window_extent.height = (uint32_t)window_height;

    /*****************************************************************************************************************/
    // Window Surface Creation
    /*****************************************************************************************************************/

    if (SDL_Vulkan_CreateSurface(window->sdl_window_handle, vulkan_instance, NULL, &window->surface) == false)
    {
        log_error("Failed to create window surface: %s", SDL_GetError());
        window_destroy(window);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Surface Format Selection
    /*****************************************************************************************************************/

    uint32_t surface_format_count;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, window->surface, &surface_format_count, NULL);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to query surface format count: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    if (surface_format_count == 0)
    {
        log_error("No surface format found");
        window_destroy(window);
        return NULL;
    }

    VkSurfaceFormatKHR *surface_formats =
    (
        (VkSurfaceFormatKHR *)malloc(surface_format_count * sizeof(VkSurfaceFormatKHR))
    );

    if (surface_formats == NULL)
    {
        log_error("Failed to allocate memory for surface formats");
        window_destroy(window);
        return NULL;
    }

    result = vkGetPhysicalDeviceSurfaceFormatsKHR
    (
        physical_device, window->surface, &surface_format_count, surface_formats
    );

    if (result != VK_SUCCESS)
    {
        log_error("Failed to query surface formats: %s", string_VkResult(result));
        window_destroy(window);
        free(surface_formats);
        return NULL;
    }

    uint32_t chosen_surface_format_index = UINT32_MAX;
    for (uint32_t surface_format_index = 0; surface_format_index < surface_format_count; ++surface_format_index)
    {
        VkSurfaceFormatKHR surface_format = surface_formats[surface_format_index];
        
        if
        (
            surface_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        )
        {
            chosen_surface_format_index = surface_format_index;
            break;
        }
    }

    if (chosen_surface_format_index == UINT32_MAX)
    {
        log_error("Failed to find suitable surface format");
        window_destroy(window);
        free(surface_formats);
        return NULL;
    }

    window->surface_format = surface_formats[chosen_surface_format_index];
    free(surface_formats);

    /*****************************************************************************************************************/
    // Swapchain Creation
    /*****************************************************************************************************************/

    VkSwapchainCreateInfoKHR swapchain_creation_information = {};
    swapchain_creation_information.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_creation_information.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_creation_information.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_creation_information.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_creation_information.imageArrayLayers = 1;
    swapchain_creation_information.clipped = VK_TRUE;
    swapchain_creation_information.oldSwapchain = VK_NULL_HANDLE;
    swapchain_creation_information.surface = window->surface;
    swapchain_creation_information.imageFormat = window->surface_format.format;
    swapchain_creation_information.imageColorSpace = window->surface_format.colorSpace;
    swapchain_creation_information.imageExtent = window->window_extent;

    VkSurfaceCapabilitiesKHR surface_capabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, window->surface, &surface_capabilities);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to query physical device surface capabilities: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    swapchain_creation_information.minImageCount = surface_capabilities.minImageCount;
    swapchain_creation_information.preTransform = surface_capabilities.currentTransform;

    result = vkCreateSwapchainKHR(logical_device, &swapchain_creation_information, NULL, &window->swapchain);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to create swapchain: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Fetching Swapchain Images
    /*****************************************************************************************************************/

    result = vkGetSwapchainImagesKHR(logical_device, window->swapchain, &window->image_count, NULL);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to query swapchain image count: %d", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }
      
    if (window->image_count == 0)
    {
        log_error("No images found in swapchain");
        window_destroy(window);
        return NULL;
    }

    window->images = (VkImage *)malloc(window->image_count * sizeof(VkImage));
    if (window->images == NULL)
    {
        log_error("Failed to allocate memory for swapchain images");
        window_destroy(window);
        return NULL;
    }
    
    result = vkGetSwapchainImagesKHR(logical_device, window->swapchain, &window->image_count, window->images);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to query swapchain images: %d", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Image Views Creation
    /*****************************************************************************************************************/

    window->image_views = (VkImageView *)malloc(window->image_count * sizeof(VkImageView));
    if (window->image_views == NULL)
    {
        log_error("Failed to allocate memory for image views");
        window_destroy(window);
        return NULL;
    }

    for (uint32_t image_view_index = 0; image_view_index < window->image_count; ++image_view_index)
    {
        VkImageViewCreateInfo image_view_creation_information = {};
        image_view_creation_information.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_creation_information.image = window->images[image_view_index];
        image_view_creation_information.format = window->surface_format.format;
        image_view_creation_information.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_creation_information.subresourceRange.baseMipLevel = 0;
        image_view_creation_information.subresourceRange.levelCount = 1;
        image_view_creation_information.subresourceRange.baseArrayLayer = 0;
        image_view_creation_information.subresourceRange.layerCount = 1;
        image_view_creation_information.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_creation_information.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_creation_information.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_creation_information.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        image_view_creation_information.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        result = vkCreateImageView
        (
            logical_device, &image_view_creation_information, NULL, &window->image_views[image_view_index]
        );

        if (result != VK_SUCCESS)
        {
            log_error("Failed to create image view %d: %s", image_view_index, string_VkResult(result));
            window_destroy(window);
            return NULL;
        }
    }

    /*****************************************************************************************************************/
    // Render Pass Creation
    /*****************************************************************************************************************/

    VkAttachmentDescription color_attachment_description = {};
    color_attachment_description.format = window->surface_format.format;
    color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_reference = {};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = {};
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &color_attachment_reference;

    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_creation_information = {};
    render_pass_creation_information.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_creation_information.attachmentCount = 1;
    render_pass_creation_information.pAttachments = &color_attachment_description;
    render_pass_creation_information.subpassCount = 1;
    render_pass_creation_information.pSubpasses = &subpass_description;
    render_pass_creation_information.dependencyCount = 1;
    render_pass_creation_information.pDependencies = &subpass_dependency;

    result = vkCreateRenderPass(logical_device, &render_pass_creation_information, NULL, &window->render_pass);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to create render pass: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Pipeline Creation
    /*****************************************************************************************************************/

    const char *shader_file_paths[] =
    {
        "resources/shaders/shader.vert.spv",
        "resources/shaders/shader.frag.spv"
    };

    window->shader_module_count = sizeof(shader_file_paths) / sizeof(shader_file_paths[0]);
    window->shader_modules = (VkShaderModule *)malloc(window->shader_module_count * sizeof(VkShaderModule));
    for (size_t shader_module_index = 0; shader_module_index < window->shader_module_count; ++shader_module_index)
    {
        const char *shader_file_path = shader_file_paths[shader_module_index];

        FILE *file = fopen(shader_file_path, "rb");
        if (file == NULL)
        {
            log_error("Failed to open shader file with path: %s", shader_file_path);
            window_destroy(window);
            return NULL;
        }

        if (fseek(file, 0, SEEK_END) != 0)
        {
            fclose(file);
            log_error("Failed to seek to end of shader file with path: %s", shader_file_path);
            window_destroy(window);
            return NULL;
        }

        long size = ftell(file);
        if (size == -1L)
        {
            fclose(file);
            log_error("Failed to determine shader file size with path: %s", shader_file_path);
            window_destroy(window);
            return NULL;
        }

        if (fseek(file, 0, SEEK_SET) != 0)
        {
            fclose(file);
            log_error("Failed to rewind shader file with path: %s", shader_file_path);
            window_destroy(window);
            return NULL;
        }

        if (size % 4 != 0)
        {
            fclose(file);
            log_error("Shader file size is not aligned: %s", shader_file_path);
            window_destroy(window);
            return NULL;
        }

        uint32_t *buffer = (uint32_t *)malloc(size);
        if (buffer == NULL)
        {
            fclose(file);
            log_error("Failed to allocate memory for shader file buffer");
            window_destroy(window);
            return NULL;
        }

        if (fread(buffer, 1, size, file) != size)
        {
            free(buffer);
            fclose(file);
            log_error("Failed to read full shader file: %s", shader_file_path);
            window_destroy(window);
            return NULL;
        }

        fclose(file);

        VkShaderModuleCreateInfo shader_module_creation_information = {};
        shader_module_creation_information.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_creation_information.codeSize = size;
        shader_module_creation_information.pCode = buffer;

        result = vkCreateShaderModule
        (
            logical_device, &shader_module_creation_information, NULL, &window->shader_modules[shader_module_index]
        );

        free(buffer);

        if (result != VK_SUCCESS)
        {
            log_error("Failed to create shader module: %s", string_VkResult(result));
            window_destroy(window);
            return NULL;
        }
    }

    VkPipelineLayoutCreateInfo pipeline_layout_creation_information = {};
    pipeline_layout_creation_information.sType  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_creation_information.setLayoutCount = 0;
    pipeline_layout_creation_information.pSetLayouts = NULL;
    pipeline_layout_creation_information.pushConstantRangeCount = 0;
    pipeline_layout_creation_information.pPushConstantRanges = NULL;

    result = vkCreatePipelineLayout
    (
        logical_device, &pipeline_layout_creation_information, NULL, &window->pipeline_layout
    );

    if (result != VK_SUCCESS)
    {
        log_error("Failed to create pipeline layout: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    VkPipelineShaderStageCreateInfo vertex_shader_stage_creation_information = {};
    vertex_shader_stage_creation_information.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_creation_information.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_creation_information.pName = "main";
    vertex_shader_stage_creation_information.module = window->shader_modules[0];

    VkPipelineShaderStageCreateInfo fragment_shader_stage_creation_information = {};
    fragment_shader_stage_creation_information.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragment_shader_stage_creation_information.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragment_shader_stage_creation_information.pName = "main";
    fragment_shader_stage_creation_information.module = window->shader_modules[1];

    VkPipelineShaderStageCreateInfo shader_stages_creation_information[2];
    shader_stages_creation_information[0] = vertex_shader_stage_creation_information;
    shader_stages_creation_information[1] = fragment_shader_stage_creation_information;

    VkPipelineVertexInputStateCreateInfo vertex_input_state_creation_information = {};
    vertex_input_state_creation_information.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_creation_information.vertexBindingDescriptionCount = 0;
    vertex_input_state_creation_information.pVertexBindingDescriptions = NULL;
    vertex_input_state_creation_information.vertexAttributeDescriptionCount = 0;
    vertex_input_state_creation_information.pVertexAttributeDescriptions = NULL;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_creation_information = {};
    input_assembly_state_creation_information.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_creation_information.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_creation_information.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state_creation_information = {};
    viewport_state_creation_information.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)window_width;
    viewport.height = (float)window_height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = window_width;
    scissor.extent.height = window_height;

    viewport_state_creation_information.pViewports = &viewport;
    viewport_state_creation_information.viewportCount = 1;
    viewport_state_creation_information.pScissors = &scissor;
    viewport_state_creation_information.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_state_creation_information = {};
    rasterization_state_creation_information.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_creation_information.depthClampEnable = VK_FALSE;
    rasterization_state_creation_information.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_creation_information.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_creation_information.lineWidth = 1.0f;
    rasterization_state_creation_information.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_state_creation_information.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state_creation_information.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling_state_creation_information = {};
    multisampling_state_creation_information.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_state_creation_information.sampleShadingEnable = VK_FALSE;
    multisampling_state_creation_information.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.blendEnable = VK_FALSE;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.colorWriteMask =
    (
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT
    );

    VkPipelineColorBlendStateCreateInfo color_blend_state_creation_information = {};
    color_blend_state_creation_information.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_creation_information.logicOpEnable = VK_FALSE;
    color_blend_state_creation_information.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_creation_information.attachmentCount = 1;
    color_blend_state_creation_information.pAttachments = &color_blend_attachment_state;

    VkGraphicsPipelineCreateInfo pipeline_creation_information = {};
    pipeline_creation_information.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_creation_information.pStages = shader_stages_creation_information;
    pipeline_creation_information.stageCount = 2;

    pipeline_creation_information.pVertexInputState = &vertex_input_state_creation_information;
    pipeline_creation_information.pInputAssemblyState = &input_assembly_state_creation_information;
    pipeline_creation_information.pViewportState = &viewport_state_creation_information;
    pipeline_creation_information.pRasterizationState = &rasterization_state_creation_information;
    pipeline_creation_information.pMultisampleState = &multisampling_state_creation_information;
    pipeline_creation_information.pColorBlendState = &color_blend_state_creation_information;
    pipeline_creation_information.pDepthStencilState = NULL;
    pipeline_creation_information.pDynamicState = NULL;
    pipeline_creation_information.layout = window->pipeline_layout;
    pipeline_creation_information.renderPass = window->render_pass;
    pipeline_creation_information.subpass = 0;

    result = vkCreateGraphicsPipelines
    (
        logical_device, VK_NULL_HANDLE, 1, &pipeline_creation_information, NULL, &window->pipeline
    );

    if (result != VK_SUCCESS)
    {
        log_error("Failed to create pipeline: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Framebuffers Creation
    /*****************************************************************************************************************/

    window->framebuffers = (VkFramebuffer *)malloc(window->image_count * sizeof(VkFramebuffer));
    if (window->framebuffers == NULL)
    {
        log_error("Failed to allocate memory for framebuffers");
        window_destroy(window);
        return NULL;
    }

    for (uint32_t framebuffer_index = 0; framebuffer_index < window->image_count; ++framebuffer_index)
    {
        VkFramebufferCreateInfo framebuffer_creation_information = {};
        framebuffer_creation_information.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_creation_information.renderPass = window->render_pass;
        framebuffer_creation_information.attachmentCount = 1;
        framebuffer_creation_information.pAttachments = &window->image_views[framebuffer_index];
        framebuffer_creation_information.width = (uint32_t)window_width;
        framebuffer_creation_information.height = (uint32_t)window_height;
        framebuffer_creation_information.layers = 1;

        result = vkCreateFramebuffer
        (
            logical_device, &framebuffer_creation_information, NULL, &window->framebuffers[framebuffer_index]
        );

        if (result != VK_SUCCESS)
        {
            log_error("Failed to create framebuffer %d: %s", framebuffer_index, string_VkResult(result));
            window_destroy(window);
            return NULL;
        }
    }

    /*****************************************************************************************************************/
    // Command Pool Creation
    /*****************************************************************************************************************/

    VkCommandPoolCreateInfo command_pool_creation_information = {};
    command_pool_creation_information.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_creation_information.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_creation_information.queueFamilyIndex = context->graphics_queue_family_index;

    result = vkCreateCommandPool(logical_device, &command_pool_creation_information, NULL, &window->command_pool);
    if (result)
    {
        log_error("Failed to create command pool: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    return window;
}

/*********************************************************************************************************************/
// Window Destruction Function
/*********************************************************************************************************************/

void window_destroy(Window *window)
{
    if (window == NULL)
    {
        return;
    }

    VkInstance vulkan_instance = window->context->vulkan_instance;
    VkDevice logical_device = window->context->logical_device;

    vkDeviceWaitIdle(logical_device);

    for (uint32_t framebuffer_index = 0; framebuffer_index < window->image_count; ++framebuffer_index)
    {
        vkDestroyFramebuffer(logical_device, window->framebuffers[framebuffer_index], NULL);
    }

    free(window->framebuffers);

    vkDestroyPipeline(logical_device, window->pipeline, NULL);
    vkDestroyPipelineLayout(logical_device, window->pipeline_layout, NULL);

    for (size_t shader_module_index = 0; shader_module_index < window->shader_module_count; ++shader_module_index)
    {
        vkDestroyShaderModule(logical_device, window->shader_modules[shader_module_index], NULL);
    }

    free(window->shader_modules);

    vkDestroyRenderPass(logical_device, window->render_pass, NULL);

    for (uint32_t image_view_index = 0; image_view_index < window->image_count; ++image_view_index)
    {
        vkDestroyImageView(logical_device, window->image_views[image_view_index], NULL);
    }

    free(window->image_views);
    free(window->images);

    vkDestroySwapchainKHR(logical_device, window->swapchain, NULL);
    SDL_Vulkan_DestroySurface(vulkan_instance, window->surface, NULL);
    SDL_DestroyWindow(window->sdl_window_handle);
    free(window);
}