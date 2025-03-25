#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <engine/math.h>
#include <engine/files.h>
#include <engine/platform.h>
#include <engine/logging.h>

#include <engine/graphics.h>

/*********************************************************************************************************************/
// Context and Window Definitions
/*********************************************************************************************************************/

struct Context
{
    VkInstance vulkan_instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice physical_device;
    uint64_t graphics_queue_family_index;
    uint64_t present_queue_family_index;
    VkDevice logical_device;
    VkQueue graphics_queue;
    VkQueue present_queue;
};

struct Window
{
    const struct Context *context;
    SDL_Window *sdl_window_handle;
    VkExtent2D window_extent;
    VkSurfaceKHR surface;
    VkSurfaceFormatKHR surface_format;
    VkSwapchainKHR swapchain;
    uint32_t image_count;
    VkImage *images;
    VkImageView *image_views;
    VkRenderPass render_pass;
    uint32_t shader_module_count;
    VkShaderModule *shader_modules;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    uint32_t maximum_frames_in_flight;
    VkCommandPool command_pool;
    VkCommandBuffer *command_buffers;
    VkImage depth_image;
    VkDeviceMemory depth_image_memory;
    VkImageView depth_image_view;
    VkImage texture_image;
    VkDeviceMemory texture_image_memory;
    VkImageView texture_image_view;
    VkSampler texture_sampler;
    uint32_t vertex_count;
    float *vertices;
    uint32_t index_count;
    uint32_t *indices;
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;
    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;
    VkBuffer *uniform_buffers;
    VkDeviceMemory *uniform_buffers_memory;
    void **uniform_buffers_mapped;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet *descriptor_sets;
    VkFramebuffer *framebuffers;
    VkSemaphore *image_available_semaphores;
    VkSemaphore *render_finished_semaphores;
    VkFence *in_flight_fences;
    uint32_t image_index;
    uint32_t frame_index;
    struct UniformBufferObject
    {
        float model[4][4];
        float view[4][4];
        float projection[4][4];
    } uniform_buffer_object;
};

/*********************************************************************************************************************/
// Context Functions
/*********************************************************************************************************************/

static inline VkBool32 vulkan_debug_messenger_callback
(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity_flags, VkDebugUtilsMessageTypeFlagsEXT message_type_flags,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data
);

static uint32_t rate_physical_device_properties(const VkPhysicalDevice physical_device);
static uint32_t rate_physical_device_features(const VkPhysicalDevice physical_device);
static uint32_t rate_physical_device_memory(const VkPhysicalDevice physical_device);
static uint32_t rate_physical_device_queue_families(const VkPhysicalDevice physical_device);
static uint32_t rate_physical_device_extensions(const VkPhysicalDevice physical_device);

/*********************************************************************************************************************/
// Context Creation Function
/*********************************************************************************************************************/

struct Context *context_create()
{
    VkResult result;

    struct Context *const context = calloc(1, sizeof(struct Context));
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
    const char *const instance_layers[] = {};
#else
    /*
        "VK_LAYER_KHRONOS_validation"
        For reporting error messages about potential API misuse and, performance issues, or invalid synchronization
    */
    const char *const instance_layers[] =
    {
        "VK_LAYER_KHRONOS_validation"
    };
#endif

    const size_t instance_layer_count = sizeof(instance_layers) / sizeof(instance_layers[0]);
    instance_creation_information.enabledLayerCount = instance_layer_count;
    instance_creation_information.ppEnabledLayerNames = instance_layers;

    uint32_t sdl_instance_extension_count = 0;
    const char *const *const sdl_instance_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_instance_extension_count);
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
    const char *const additional_instance_extensions[] =
    {
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
#else
    /*
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        For setting up a debug messenger associated with the Vulkan instance
    */
    const char *const additional_instance_extensions[] =
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
#endif

    const size_t additional_instance_extension_count =
    (
        sizeof(additional_instance_extensions) / sizeof(additional_instance_extensions[0])
    );

    const size_t instance_extension_count = sdl_instance_extension_count + additional_instance_extension_count;
    const char **const instance_extensions = malloc(instance_extension_count * sizeof(const char *));
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

    VkPhysicalDevice *const physical_devices = malloc(physical_device_count * sizeof(VkPhysicalDevice));
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
        VkPhysicalDevice const physical_device = physical_devices[physical_device_index];
        const uint32_t score =
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

    VkQueueFamilyProperties *const queue_families = malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
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
    const char *const logical_device_layers[] = {};
#else
    /*
        "VK_LAYER_KHRONOS_validation"
        For reporting error messages about potential API misuse and, performance issues, or invalid synchronization
    */
    const char *const logical_device_layers[] =
    {
        "VK_LAYER_KHRONOS_validation"
    };
#endif

    const size_t logical_device_layer_count = sizeof(logical_device_layers) / sizeof(logical_device_layers[0]);

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
    const char *const logical_device_extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
    };
#else
    /*
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
        or presenting on a window surface
    */
    const char *const logical_device_extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
#endif

    const size_t logical_device_extension_count = sizeof(logical_device_extensions) / sizeof(logical_device_extensions[0]);

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

void context_destroy(struct Context *const context)
{
    if (context == NULL)
    {
        return;
    }

    VkInstance const vulkan_instance = context->vulkan_instance;

    vkDestroyDevice(context->logical_device, NULL);

    if (vulkan_instance != NULL)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT debug_messenger_destruction_function =
        (
            (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(vulkan_instance, "vkDestroyDebugUtilsMessengerEXT")
        );

        if (debug_messenger_destruction_function)
        {
            debug_messenger_destruction_function(vulkan_instance, context->debug_messenger, NULL);
        }
        else
        {
            log_warning("Failed to query debug messenger destruction function");
        }
    }

    vkDestroyInstance(vulkan_instance, NULL);
    free(context);
}

/*********************************************************************************************************************/
// Vulkan Debug Messenger Callback Function
/*********************************************************************************************************************/

static inline VkBool32 vulkan_debug_messenger_callback
(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity_flags, VkDebugUtilsMessageTypeFlagsEXT message_type_flags,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *user_data
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
            const size_t length = strlen(type_string);
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
            const size_t length = strlen(type_string);
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
            const size_t length = strlen(type_string);
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

static uint32_t rate_physical_device_properties(const VkPhysicalDevice physical_device)
{
    uint32_t score = 0;

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(physical_device, &device_properties);

    // It would be nice to have the physical device support Vulkan version 1.3 at least
    const uint32_t api_version = device_properties.apiVersion;
    const uint32_t major_version = VK_VERSION_MAJOR(api_version);
    const uint32_t minor_version = VK_VERSION_MINOR(api_version);
    if (major_version >= 2 || (major_version == 1 && minor_version >= 3))
    {
        score += 500;
    }

    if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }

    const VkPhysicalDeviceLimits limits = device_properties.limits;
    score += limits.maxImageDimension2D;

    return score;
}

static uint32_t rate_physical_device_features(const VkPhysicalDevice physical_device)
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

static uint32_t rate_physical_device_memory(const VkPhysicalDevice physical_device)
{
    uint32_t score = 0;

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    uint32_t total_memory = 0;
    for (uint32_t memory_heap_index = 0; memory_heap_index < memory_properties.memoryHeapCount; memory_heap_index++)
    {
        const VkMemoryHeap memory_heap = memory_properties.memoryHeaps[memory_heap_index];
        if (memory_heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            // The total memory will be converted to megabytes
            total_memory += memory_heap.size / (1024 * 1024);
        }
    }

    return total_memory / 10;
}

static uint32_t rate_physical_device_queue_families(const VkPhysicalDevice physical_device)
{
    uint32_t score = 0;

    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);

    VkQueueFamilyProperties *const queue_families = malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
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

static uint32_t rate_physical_device_extensions(const VkPhysicalDevice physical_device)
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

    VkExtensionProperties *const extensions = malloc(extension_count * sizeof(VkExtensionProperties));
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
        const char *const extension_name = extensions[extension_index].extensionName;
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
// Window Functions
/*********************************************************************************************************************/

static inline uint8_t window_begin_frame(struct Window *const window);
static inline uint8_t window_end_frame(const struct Window *const window);

static inline uint8_t memory_allocate
(
    const struct Window *const window, const VkMemoryRequirements memory_requirements,
    const VkMemoryPropertyFlags memory_properties, VkDeviceMemory *memory
);

static inline uint8_t command_start(const struct Window *const window, VkCommandBuffer *command_buffer);
static inline uint8_t command_finish(const struct Window *const window, VkCommandBuffer command_buffer);

static inline uint8_t buffer_create
(
    const struct Window *const window, const VkDeviceSize buffer_size, const VkBufferUsageFlags buffer_usages,
    const VkMemoryPropertyFlags memory_properties, VkBuffer *buffer, VkDeviceMemory *buffer_memory
);

static inline uint8_t buffer_copy
(
    const struct Window *const window, VkBuffer source_buffer, VkBuffer destination_buffer,
    const VkDeviceSize buffer_size
);

static inline uint8_t image_load
(
    const struct Window *const window, const char *const file_path, VkImage *texture_image,
    VkDeviceMemory *texture_image_memory
);

/*********************************************************************************************************************/
// Window Creation Function
/*********************************************************************************************************************/

struct Window *window_create
(
    const char *const title, const unsigned int initial_width, const unsigned int initial_height,
    const struct Context *context
)
{
    VkResult result;

    if (context == NULL)
    {
        log_error("Cannot create 'Window' if given 'Context' is 'NULL'");
        return NULL;
    }

    struct Window *const window = calloc(1, sizeof(struct Window));
    if (window == NULL)
    {
        log_error("Failed to allocate memory for window instance");
        return NULL;
    }

    window->context = context;
    VkInstance const vulkan_instance = context->vulkan_instance;
    VkPhysicalDevice const physical_device = context->physical_device;
    VkDevice const logical_device = context->logical_device;

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

    VkSurfaceFormatKHR *const surface_formats = malloc(surface_format_count * sizeof(VkSurfaceFormatKHR));

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

    window->images = calloc(window->image_count, sizeof(VkImage));
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

    window->image_views = calloc(window->image_count, sizeof(VkImageView));
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
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment_reference.attachment = 0;

    const VkFormat depth_formats[] =
    {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    size_t chosen_depth_format_index = SIZE_MAX;
    const size_t depth_format_count = sizeof(depth_formats) / sizeof(depth_formats[0]);
    for (size_t depth_format_index = 0; depth_format_index < depth_format_count; ++depth_format_index)
    {
        VkFormatProperties format_properties;
        vkGetPhysicalDeviceFormatProperties(physical_device, depth_formats[depth_format_index], &format_properties);

        if
        (
            (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        )
        {
            chosen_depth_format_index = depth_format_index;
            break;
        }
    }

    if (chosen_depth_format_index == SIZE_MAX)
    {
        log_error("Failed to find supported depth format");
        window_destroy(window);
        return NULL;
    }

    const VkFormat depth_format = depth_formats[chosen_depth_format_index];

    VkAttachmentDescription depth_attachment_description = {};
    depth_attachment_description.format = depth_format;
    depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference = {};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = {};
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
    subpass_description.pColorAttachments = &color_attachment_reference;
    subpass_description.colorAttachmentCount = 1;

    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask =
    (
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
    );

    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstStageMask =
    (
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
    );

    subpass_dependency.dstAccessMask =
    (
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    );

    const VkAttachmentDescription attachment_descriptions[] =
    {
        color_attachment_description,
        depth_attachment_description
    };

    const size_t attachment_description_count = sizeof(attachment_descriptions) / sizeof(attachment_descriptions[0]);

    VkRenderPassCreateInfo render_pass_creation_information = {};
    render_pass_creation_information.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_creation_information.attachmentCount = attachment_description_count;
    render_pass_creation_information.pAttachments = attachment_descriptions;
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
    // Shader Modules Creation
    /*****************************************************************************************************************/

    const char *const shader_file_paths[] =
    {
        "resources/shaders/shader.vert.spv",
        "resources/shaders/shader.frag.spv"
    };

    window->shader_module_count = sizeof(shader_file_paths) / sizeof(shader_file_paths[0]);
    window->shader_modules = calloc(window->shader_module_count, sizeof(VkShaderModule));
    for (size_t shader_module_index = 0; shader_module_index < window->shader_module_count; ++shader_module_index)
    {
        const char *const shader_file_path = shader_file_paths[shader_module_index];
        struct Shader *const shader = shader_create(shader_file_path);
        if (shader == NULL)
        {
            log_error("Failed to create shader: %s", shader_file_path);
            window_destroy(window);
            return NULL;
        }

        VkShaderModuleCreateInfo shader_module_creation_information = {};
        shader_module_creation_information.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_creation_information.codeSize = shader->size;
        shader_module_creation_information.pCode = shader->data;

        result = vkCreateShaderModule
        (
            logical_device, &shader_module_creation_information, NULL, &window->shader_modules[shader_module_index]
        );

        shader_destroy(shader);

        if (result != VK_SUCCESS)
        {
            log_error("Failed to create shader module: %s", string_VkResult(result));
            window_destroy(window);
            return NULL;
        }
    }

    /*****************************************************************************************************************/
    // Descriptor Set Layout Creation
    /*****************************************************************************************************************/

    VkDescriptorSetLayoutBinding uniform_buffer_object_descriptor_set_layout_binding = {};
    uniform_buffer_object_descriptor_set_layout_binding.binding = 0;
    uniform_buffer_object_descriptor_set_layout_binding.descriptorCount = 1;
    uniform_buffer_object_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_buffer_object_descriptor_set_layout_binding.pImmutableSamplers = NULL;
    uniform_buffer_object_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding sampler_descriptor_set_layout_binding = {};
    sampler_descriptor_set_layout_binding.binding = 1;
    sampler_descriptor_set_layout_binding.descriptorCount = 1;
    sampler_descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_descriptor_set_layout_binding.pImmutableSamplers = NULL;
    sampler_descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    const VkDescriptorSetLayoutBinding descriptor_set_layout_bindings[] =
    {
        uniform_buffer_object_descriptor_set_layout_binding,
        sampler_descriptor_set_layout_binding
    };

    const size_t descriptor_set_layout_binding_count =
    (
        sizeof(descriptor_set_layout_bindings) / sizeof(descriptor_set_layout_bindings[0])
    );

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_creation_information = {};
    descriptor_set_layout_creation_information.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_creation_information.pBindings = descriptor_set_layout_bindings;
    descriptor_set_layout_creation_information.bindingCount = descriptor_set_layout_binding_count;

    result = vkCreateDescriptorSetLayout
    (
        logical_device, &descriptor_set_layout_creation_information, NULL, &window->descriptor_set_layout
    );

    if (result != VK_SUCCESS)
    {
        log_error("Failed to create descriptor set layout: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Pipeline Layout Creation
    /*****************************************************************************************************************/

    VkPipelineLayoutCreateInfo pipeline_layout_creation_information = {};
    pipeline_layout_creation_information.sType  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_creation_information.pSetLayouts = &window->descriptor_set_layout;
    pipeline_layout_creation_information.setLayoutCount = 1;
    pipeline_layout_creation_information.pPushConstantRanges = NULL;
    pipeline_layout_creation_information.pushConstantRangeCount = 0;

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

    /*****************************************************************************************************************/
    // Pipeline Creation
    /*****************************************************************************************************************/

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

    VkVertexInputBindingDescription vertex_input_binding_description = {};
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertex_input_binding_description.stride = 8 * sizeof(float);
    vertex_input_binding_description.binding = 0;

    vertex_input_state_creation_information.pVertexBindingDescriptions = &vertex_input_binding_description;
    vertex_input_state_creation_information.vertexBindingDescriptionCount = 1;

    VkVertexInputAttributeDescription vertex_input_attribute_descriptions[3] = {};

    // Represents the position of the vertex
    vertex_input_attribute_descriptions[0].location = 0;
    vertex_input_attribute_descriptions[0].binding = 0;
    vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_descriptions[0].offset = 0;

    // Represents the color of the vertex
    vertex_input_attribute_descriptions[1].location = 1;
    vertex_input_attribute_descriptions[1].binding = 0;
    vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_descriptions[1].offset = 3 * sizeof(float);

    // Represents the position of the texture (how it is mapped onto the geometry)
    vertex_input_attribute_descriptions[2].location = 2;
    vertex_input_attribute_descriptions[2].binding = 0;
    vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_input_attribute_descriptions[2].offset = (3 + 3) * sizeof(float);

    vertex_input_state_creation_information.pVertexAttributeDescriptions = vertex_input_attribute_descriptions;
    vertex_input_state_creation_information.vertexAttributeDescriptionCount = 3;

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
    rasterization_state_creation_information.cullMode = VK_CULL_MODE_NONE;
    rasterization_state_creation_information.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state_creation_information.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling_state_creation_information = {};
    multisampling_state_creation_information.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_state_creation_information.sampleShadingEnable = VK_FALSE;
    multisampling_state_creation_information.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_creation_information = {};
    depth_stencil_state_creation_information.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_creation_information.depthTestEnable = VK_TRUE;
    depth_stencil_state_creation_information.depthWriteEnable = VK_TRUE;
    depth_stencil_state_creation_information.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_creation_information.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_creation_information.stencilTestEnable = VK_FALSE;
    depth_stencil_state_creation_information.minDepthBounds = 0.0f;
    depth_stencil_state_creation_information.maxDepthBounds = 1.0f;

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
    pipeline_creation_information.pDepthStencilState = &depth_stencil_state_creation_information;
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
    // Command Pool Creation
    /*****************************************************************************************************************/

    VkCommandPoolCreateInfo command_pool_creation_information = {};
    command_pool_creation_information.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_creation_information.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_creation_information.queueFamilyIndex = context->graphics_queue_family_index;

    result = vkCreateCommandPool(logical_device, &command_pool_creation_information, NULL, &window->command_pool);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to create command pool: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Command Buffers Creation
    /*****************************************************************************************************************/

    // For now, set the maximum number of frames in flight to two
    window->maximum_frames_in_flight = 2;

    VkCommandBufferAllocateInfo command_buffer_allocation_information = {};
    command_buffer_allocation_information.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocation_information.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocation_information.commandPool = window->command_pool;
    command_buffer_allocation_information.commandBufferCount = window->maximum_frames_in_flight;

    window->command_buffers = calloc(window->maximum_frames_in_flight, sizeof(VkCommandBuffer));
    if (window->command_buffers == NULL)
    {
        log_error("Failed to allocate memory for command buffers");
        window_destroy(window);
        return NULL;
    }

    result = vkAllocateCommandBuffers
    (
        logical_device, &command_buffer_allocation_information, window->command_buffers
    );

    if (result != VK_SUCCESS)
    {
        log_error("Failed to create command buffers: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Depth Resources Creation
    /*****************************************************************************************************************/

    VkImageCreateInfo depth_image_creation_information = {};
    depth_image_creation_information.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depth_image_creation_information.imageType = VK_IMAGE_TYPE_2D;
    depth_image_creation_information.extent.width = window->window_extent.width;
    depth_image_creation_information.extent.height = window->window_extent.height;
    depth_image_creation_information.extent.depth = 1;
    depth_image_creation_information.mipLevels = 1;
    depth_image_creation_information.arrayLayers = 1;
    depth_image_creation_information.format = depth_format;
    depth_image_creation_information.tiling = VK_IMAGE_TILING_OPTIMAL;
    depth_image_creation_information.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_image_creation_information.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depth_image_creation_information.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_image_creation_information.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateImage(logical_device, &depth_image_creation_information, NULL, &window->depth_image);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to create depth image: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    VkMemoryRequirements depth_image_memory_requirements;
    vkGetImageMemoryRequirements(logical_device, window->depth_image, &depth_image_memory_requirements);

    if
    (
        memory_allocate
        (
            window, depth_image_memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &window->depth_image_memory
        ) == 0
    )
    {
        log_error("Failed to allocate memory for depth image");
        window_destroy(window);
        return NULL;
    }

    VkImageViewCreateInfo depth_image_view_creation_information = {};
    depth_image_view_creation_information.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depth_image_view_creation_information.image = window->depth_image;
    depth_image_view_creation_information.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depth_image_view_creation_information.format = depth_format;
    depth_image_view_creation_information.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depth_image_view_creation_information.subresourceRange.baseMipLevel = 0;
    depth_image_view_creation_information.subresourceRange.levelCount = 1;
    depth_image_view_creation_information.subresourceRange.baseArrayLayer = 0;
    depth_image_view_creation_information.subresourceRange.layerCount = 1;

    result = vkCreateImageView
    (
        logical_device, &depth_image_view_creation_information, NULL, &window->depth_image_view
    );

    if (result != VK_SUCCESS)
    {
        log_error("Failed to create depth image view: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Texture Image Creation
    /*****************************************************************************************************************/

    const char *const image_file_path = "resources/images/textures/squares.png";
    if (image_load(window, image_file_path, &window->texture_image, &window->texture_image_memory) == 0)
    {
        log_error("Failed to load texture image");
        window_destroy(window);
        return NULL;
    }

    VkImageViewCreateInfo image_view_creation_information = {};
    image_view_creation_information.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_creation_information.image = window->texture_image;
    image_view_creation_information.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_creation_information.format = VK_FORMAT_R8G8B8A8_SRGB;
    image_view_creation_information.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_creation_information.subresourceRange.baseMipLevel = 0;
    image_view_creation_information.subresourceRange.levelCount = 1;
    image_view_creation_information.subresourceRange.baseArrayLayer = 0;
    image_view_creation_information.subresourceRange.layerCount = 1;

    result = vkCreateImageView(logical_device, &image_view_creation_information, NULL, &window->texture_image_view);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to create texture image view: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    VkSamplerCreateInfo sampler_creation_information = {};
    sampler_creation_information.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_creation_information.magFilter = VK_FILTER_LINEAR;
    sampler_creation_information.minFilter = VK_FILTER_LINEAR;
    sampler_creation_information.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_creation_information.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_creation_information.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_creation_information.anisotropyEnable = VK_FALSE;
    sampler_creation_information.maxAnisotropy = 1.0f;
    sampler_creation_information.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_creation_information.unnormalizedCoordinates = VK_FALSE;
    sampler_creation_information.compareEnable = VK_FALSE;
    sampler_creation_information.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_creation_information.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_creation_information.mipLodBias = 0.0f;
    sampler_creation_information.minLod = 0.0f;
    sampler_creation_information.maxLod = 0.0f;

    result = vkCreateSampler(logical_device, &sampler_creation_information, NULL, &window->texture_sampler);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to create texture sampler: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Loading Vertices and Indices
    /*****************************************************************************************************************/

    window->vertex_count = 8;
    window->vertices = malloc(8 * window->vertex_count * sizeof(float));
    if (window->vertices == NULL)
    {
        log_error("Failed to allocate memory for vertices");
        window_destroy(window);
        return NULL;
    }

    const float vertices[] =
    {
        -0.5f, -0.5f, 0.25f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.25f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, 0.25f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,

        -0.5f, -0.5f, -0.25f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.25f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, -0.25f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, -0.25f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f
    };

    memcpy(window->vertices, &vertices, sizeof(vertices));

    window->index_count = 12;
    window->indices = malloc(window->index_count * sizeof(unsigned short));
    if (window->indices == NULL)
    {
        log_error("Failed to allocate memory for indices");
        window_destroy(window);
        return NULL;
    }

    const uint16_t indices[] =
    {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

    memcpy(window->indices, &indices, sizeof(indices));

    /*****************************************************************************************************************/
    // Vertex Buffer Creation
    /*****************************************************************************************************************/

    const VkDeviceSize vertex_buffer_size = 8 * window->vertex_count * sizeof(float);

    VkBuffer vertex_staging_buffer;
    VkDeviceMemory vertex_staging_buffer_memory;

    if
    (
        buffer_create
        (
            window, vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertex_staging_buffer,
            &vertex_staging_buffer_memory
        ) == 0
    )
    {
        log_error("Failed to create vertex staging buffer");
        window_destroy(window);
        return NULL;
    }

    void *vertex_staging_buffer_data;
    result = vkMapMemory
    (
        logical_device, vertex_staging_buffer_memory, 0, vertex_buffer_size, 0, &vertex_staging_buffer_data
    );

    if (result != VK_SUCCESS)
    {
        log_error("Failed to map staging buffer memory: %s", string_VkResult(result));
        vkDestroyBuffer(logical_device, vertex_staging_buffer, NULL);
        vkFreeMemory(logical_device, vertex_staging_buffer_memory, NULL);
        window_destroy(window);
        return NULL;
    }

    memcpy(vertex_staging_buffer_data, window->vertices, vertex_buffer_size);
    vkUnmapMemory(logical_device, vertex_staging_buffer_memory);

    if
    (
        buffer_create
        (
            window, vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &window->vertex_buffer, &window->vertex_buffer_memory
        ) == 0
    )
    {
        log_error("Failed to create vertex buffer");
        vkDestroyBuffer(logical_device, vertex_staging_buffer, NULL);
        vkFreeMemory(logical_device, vertex_staging_buffer_memory, NULL);
        window_destroy(window);
        return NULL;
    }

    if (buffer_copy(window, vertex_staging_buffer, window->vertex_buffer, vertex_buffer_size) == 0)
    {
        log_error("Failed to copy staging buffer onto vertex buffer");
        vkDestroyBuffer(logical_device, vertex_staging_buffer, NULL);
        vkFreeMemory(logical_device, vertex_staging_buffer_memory, NULL);
        window_destroy(window);
        return NULL;
    }

    vkDestroyBuffer(logical_device, vertex_staging_buffer, NULL);
    vkFreeMemory(logical_device, vertex_staging_buffer_memory, NULL);

    /*****************************************************************************************************************/
    // Index Buffer Creation
    /*****************************************************************************************************************/

    const VkDeviceSize index_buffer_size = window->index_count * sizeof(unsigned short);

    VkBuffer index_staging_buffer;
    VkDeviceMemory index_staging_buffer_memory;

    if
    (
        buffer_create
        (
            window, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &index_staging_buffer,
            &index_staging_buffer_memory
        ) == 0
    )
    {
        log_error("Failed to create index staging buffer");
        window_destroy(window);
        return NULL;
    }

    void *index_staging_buffer_data;
    result = vkMapMemory
    (
        logical_device, index_staging_buffer_memory, 0, index_buffer_size, 0, &index_staging_buffer_data
    );

    if (result != VK_SUCCESS)
    {
        log_error("Failed to map staging buffer memory: %s", string_VkResult(result));
        vkDestroyBuffer(logical_device, index_staging_buffer, NULL);
        vkFreeMemory(logical_device, index_staging_buffer_memory, NULL);
        window_destroy(window);
        return NULL;
    }

    memcpy(index_staging_buffer_data, window->indices, index_buffer_size);
    vkUnmapMemory(logical_device, index_staging_buffer_memory);

    if
    (
        buffer_create
        (
            window, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &window->index_buffer, &window->index_buffer_memory
        ) == 0
    )
    {
        log_error("Failed to create index buffer");
        vkDestroyBuffer(logical_device, index_staging_buffer, NULL);
        vkFreeMemory(logical_device, index_staging_buffer_memory, NULL);
        window_destroy(window);
        return NULL;
    }

    if (buffer_copy(window, index_staging_buffer, window->index_buffer, index_buffer_size) == 0)
    {
        log_error("Failed to copy staging buffer onto index buffer");
        vkDestroyBuffer(logical_device, index_staging_buffer, NULL);
        vkFreeMemory(logical_device, index_staging_buffer_memory, NULL);
        window_destroy(window);
        return NULL;
    }

    vkDestroyBuffer(logical_device, index_staging_buffer, NULL);
    vkFreeMemory(logical_device, index_staging_buffer_memory, NULL);

    /*****************************************************************************************************************/
    // Uniform Buffers Creation
    /*****************************************************************************************************************/

    uint32_t uniform_buffer_count = window->maximum_frames_in_flight;
    VkDeviceSize uniform_buffer_size = sizeof(window->uniform_buffer_object);

    window->uniform_buffers = calloc(uniform_buffer_count, sizeof(VkBuffer));
    if (window->uniform_buffers == NULL)
    {
        log_error("Failed to allocate memory for uniform buffers");
        window_destroy(window);
        return NULL;
    }
    
    window->uniform_buffers_memory = calloc(uniform_buffer_count, sizeof(VkDeviceMemory));
    if (window->uniform_buffers_memory == NULL)
    {
        log_error("Failed to allocate memory for uniform buffers memory");
        window_destroy(window);
        return NULL;
    }

    window->uniform_buffers_mapped = calloc(uniform_buffer_count, sizeof(void *));
    if (window->uniform_buffers_mapped == NULL)
    {
        log_error("Failed to allocate memory for uniform buffers mapped");
        window_destroy(window);
        return NULL;
    }

    for (size_t uniform_buffer_index = 0; uniform_buffer_index < uniform_buffer_count; ++uniform_buffer_index)
    {
        if
        (
            buffer_create
            (
                window, uniform_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &window->uniform_buffers[uniform_buffer_index], &window->uniform_buffers_memory[uniform_buffer_index]
            ) == 0
        )
        {
            log_error("Failed to create uniform buffer #%d", uniform_buffer_index);
            window_destroy(window);
            return NULL;
        }

        result = vkMapMemory
        (
            logical_device, window->uniform_buffers_memory[uniform_buffer_index], 0, uniform_buffer_size, 0,
            &window->uniform_buffers_mapped[uniform_buffer_index]
        );

        if (result != VK_SUCCESS)
        {
            log_error
            (
                "Failed to map memory for uniform buffer #%d: %s", uniform_buffer_index, string_VkResult(result)
            );

            window_destroy(window);
            return NULL;
        }
    }

    /*****************************************************************************************************************/
    // Descriptor Pool Creation
    /*****************************************************************************************************************/

    VkDescriptorPoolSize descriptor_pool_sizes[2];
    descriptor_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_sizes[0].descriptorCount = (uint32_t)window->maximum_frames_in_flight;
    descriptor_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_pool_sizes[1].descriptorCount = (uint32_t)window->maximum_frames_in_flight;

    VkDescriptorPoolCreateInfo descriptor_pool_creation_information = {};
    descriptor_pool_creation_information.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_creation_information.maxSets = (uint16_t)window->maximum_frames_in_flight;
    descriptor_pool_creation_information.pPoolSizes = descriptor_pool_sizes;
    descriptor_pool_creation_information.poolSizeCount = 2;

    result = vkCreateDescriptorPool
    (
        logical_device, &descriptor_pool_creation_information, NULL, &window->descriptor_pool
    );

    if (result != VK_SUCCESS)
    {
        log_error("Failed to create descrioptor pool: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    /*****************************************************************************************************************/
    // Descriptor Sets Creation
    /*****************************************************************************************************************/

    VkDescriptorSetAllocateInfo descriptor_set_allocation_information = {};
    descriptor_set_allocation_information.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocation_information.descriptorPool = window->descriptor_pool;
    descriptor_set_allocation_information.descriptorSetCount = (uint32_t)window->maximum_frames_in_flight;

    // TODO: Address this issue:
    // I am not sure why, but it thinks that I have multiple (the number of maximum frames in flight, to be precise)
    // descriptor set layouts, and I can't get to fix this. So this array is used temporarly to fill in the array of
    // descriptor set layouts expected in `descriptor_set_allocation_information.pSetLayouts` 
    VkDescriptorSetLayout descriptor_set_layouts[window->maximum_frames_in_flight];

    for
    (
        uint32_t descriptor_set_layout_index = 0;
        descriptor_set_layout_index < window->maximum_frames_in_flight;
        ++descriptor_set_layout_index
    )
    {
        descriptor_set_layouts[descriptor_set_layout_index] = window->descriptor_set_layout;
    }

    descriptor_set_allocation_information.pSetLayouts = descriptor_set_layouts;

    window->descriptor_sets = calloc(window->maximum_frames_in_flight, sizeof(VkDescriptorSet));
    if (window->descriptor_sets == NULL)
    {
        log_error("Failed to allocate memory for descriptor sets");
        window_destroy(window);
        return NULL;
    }

    result = vkAllocateDescriptorSets(logical_device, &descriptor_set_allocation_information, window->descriptor_sets);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to allocated descriptor sets: %s", string_VkResult(result));
        window_destroy(window);
        return NULL;
    }

    for
    (
        size_t descriptor_set_index = 0;
        descriptor_set_index < window->maximum_frames_in_flight;
        ++descriptor_set_index
    )
    {
        VkDescriptorBufferInfo descriptor_buffer_information = {};
        descriptor_buffer_information.range = sizeof(window->uniform_buffer_object);
        descriptor_buffer_information.buffer = window->uniform_buffers[descriptor_set_index];
        descriptor_buffer_information.offset = 0;

        VkDescriptorImageInfo descriptor_image_information = {};
        descriptor_image_information.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptor_image_information.imageView = window->texture_image_view;
        descriptor_image_information.sampler = window->texture_sampler;

        VkDescriptorSet descriptor_set = window->descriptor_sets[descriptor_set_index];
        VkWriteDescriptorSet write_descriptor_sets[2];

        write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_sets[0].dstSet = descriptor_set;
        write_descriptor_sets[0].dstBinding = 0;
        write_descriptor_sets[0].dstArrayElement = 0;
        write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_descriptor_sets[0].descriptorCount = 1;
        write_descriptor_sets[0].pBufferInfo = &descriptor_buffer_information;
        write_descriptor_sets[0].pNext = NULL;

        write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_sets[1].dstSet = descriptor_set;
        write_descriptor_sets[1].dstBinding = 1;
        write_descriptor_sets[1].dstArrayElement = 0;
        write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_descriptor_sets[1].descriptorCount = 1;
        write_descriptor_sets[1].pImageInfo = &descriptor_image_information;
        write_descriptor_sets[1].pNext = NULL;

        vkUpdateDescriptorSets(logical_device, 2, write_descriptor_sets, 0, NULL);
    }

    /*****************************************************************************************************************/
    // Framebuffers Creation
    /*****************************************************************************************************************/

    window->framebuffers = calloc(window->image_count, sizeof(VkFramebuffer));
    if (window->framebuffers == NULL)
    {
        log_error("Failed to allocate memory for framebuffers");
        window_destroy(window);
        return NULL;
    }

    for (uint32_t framebuffer_index = 0; framebuffer_index < window->image_count; ++framebuffer_index)
    {
        const VkImageView framebuffer_attachments[] =
        {
            window->image_views[framebuffer_index],
            window->depth_image_view
        };

        const size_t framebuffer_attachment_count =
        (
            sizeof(framebuffer_attachments) / sizeof(framebuffer_attachments[0])
        );

        VkFramebufferCreateInfo framebuffer_creation_information = {};
        framebuffer_creation_information.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_creation_information.renderPass = window->render_pass;
        framebuffer_creation_information.attachmentCount = framebuffer_attachment_count;
        framebuffer_creation_information.pAttachments = framebuffer_attachments;
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
    // Synchronization Objects Creation
    /*****************************************************************************************************************/

    window->image_available_semaphores = calloc(window->maximum_frames_in_flight, sizeof(VkSemaphore));
    if (window->image_available_semaphores == NULL)
    {
        log_error("Failed to allocate memory for image available semaphores");
        window_destroy(window);
        return NULL;
    }

    window->render_finished_semaphores = calloc(window->maximum_frames_in_flight, sizeof(VkSemaphore));
    if (window->render_finished_semaphores == NULL)
    {
        log_error("Failed to allocate memory for render finished semaphores");
        window_destroy(window);
        return NULL;
    }

    window->in_flight_fences = calloc(window->maximum_frames_in_flight, sizeof(VkFence));
    if (window->in_flight_fences == NULL)
    {
        log_error("Failed to allocate memory for in flight fences");
        window_destroy(window);
        return NULL;
    }

    VkSemaphoreCreateInfo semaphore_creation_information = {};
    semaphore_creation_information.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_creation_information = {};
    fence_creation_information.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_creation_information.flags = VK_FENCE_CREATE_SIGNALED_BIT;
   
    for (uint32_t frame_index = 0; frame_index < window->maximum_frames_in_flight; ++frame_index)
    {
        result = vkCreateSemaphore
        (
            logical_device, &semaphore_creation_information, NULL, &window->image_available_semaphores[frame_index]
        );

        if (result != VK_SUCCESS)
        {
            log_error("Failed to create image available semaphore #%d: %s", frame_index, string_VkResult(result));
            window_destroy(window);
            return NULL;
        }

        result = vkCreateSemaphore
        (
            logical_device, &semaphore_creation_information, NULL, &window->render_finished_semaphores[frame_index]
        );

        if (result != VK_SUCCESS)
        {
            log_error("Failed to create render finished semaphore #%d: %s", frame_index, string_VkResult(result));
            window_destroy(window);
            return NULL;
        }

        result = vkCreateFence
        (
            logical_device, &fence_creation_information, NULL, &window->in_flight_fences[frame_index]
        );

        if (result != VK_SUCCESS)
        {
            log_error("Failed to create in flight fence #%d: %s", frame_index, string_VkResult(result));
            window_destroy(window);
            return NULL;
        }
    }

    /*****************************************************************************************************************/
    // Beginning the First Frame
    /*****************************************************************************************************************/

    if (window_begin_frame(window) == 0)
    {
        log_error("Failed to begin first window frame");
        window_destroy(window);
        return NULL;
    }

    return window;
}

/*********************************************************************************************************************/
// Window Frame Rendering Function
/*********************************************************************************************************************/

unsigned char window_update(struct Window *const window)
{
    if (window_end_frame(window) == 0)
    {
        log_error("Failed to end window frame");
        return 0;
    }

    // Move to the next frame to get the corresponding command buffer, and synchronization objects
    window->frame_index = (window->frame_index + 1) % window->maximum_frames_in_flight;

    if (window_begin_frame(window) == 0)
    {
        log_error("Failed to begin window frame");
        return 0;
    }

    return 1;
}

static inline uint8_t window_begin_frame(struct Window *const window)
{
    VkResult result;

    VkDevice const logical_device = window->context->logical_device;

    VkCommandBuffer const command_buffer = window->command_buffers[window->frame_index];
    VkSemaphore const image_available_semaphore = window->image_available_semaphores[window->frame_index];
    VkSemaphore const render_finished_semaphore = window->render_finished_semaphores[window->frame_index];
    VkFence const in_flight_fence = window->in_flight_fences[window->frame_index];

    result = vkWaitForFences(logical_device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to wait for in flight fence #%d: %s", window->frame_index, string_VkResult(result));
        return 0;
    }

    result = vkResetFences(logical_device, 1, &in_flight_fence);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to reset in flight fence #%d: %s", window->frame_index, string_VkResult(result));
        return 0;
    }

    result = vkAcquireNextImageKHR
    (
        logical_device, window->swapchain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &window->image_index
    );

    if (result != VK_SUCCESS)
    {
        log_error("Failed to acquire next swapchain image: %s", string_VkResult(result));
        return 0;
    }

    result = vkResetCommandBuffer(command_buffer, 0);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to reset command buffer #%d: %s", window->frame_index, string_VkResult(result));
        return 0;
    }

    VkCommandBufferBeginInfo command_buffer_beginning_information = {};
    command_buffer_beginning_information.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    result = vkBeginCommandBuffer(command_buffer, &command_buffer_beginning_information);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to begin command buffer #%d: %s", window->frame_index, string_VkResult(result));
        return 0;
    }

    return 1;
}

static inline uint8_t window_end_frame(const struct Window *const window)
{
    VkResult result;

    struct Context const *context = window->context;
    VkDevice const logical_device = context->logical_device;

    VkCommandBuffer const command_buffer = window->command_buffers[window->frame_index];
    VkSemaphore const image_available_semaphore = window->image_available_semaphores[window->frame_index];
    VkSemaphore const render_finished_semaphore = window->render_finished_semaphores[window->frame_index];
    VkFence const in_flight_fence = window->in_flight_fences[window->frame_index];

    result = vkEndCommandBuffer(command_buffer);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to end command buffer #%d: %s", window->frame_index, string_VkResult(result));
        return 0;
    }

    VkSubmitInfo submission_information = {};
    submission_information.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submission_information.pCommandBuffers = &command_buffer;
    submission_information.commandBufferCount = 1;
    submission_information.pWaitSemaphores = &image_available_semaphore;
    submission_information.waitSemaphoreCount = 1;
    submission_information.pSignalSemaphores = &render_finished_semaphore;
    submission_information.signalSemaphoreCount = 1;
    
    VkPipelineStageFlags wait_stages[] =
    {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    submission_information.pWaitDstStageMask = wait_stages;

    result = vkQueueSubmit(context->graphics_queue, 1, &submission_information, in_flight_fence);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to submit command buffer #%d: %s", window->frame_index, string_VkResult(result));
        return 0;
    }

    VkPresentInfoKHR presentation_information = {};
    presentation_information.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentation_information.pSwapchains = &window->swapchain;
    presentation_information.swapchainCount = 1;
    presentation_information.pImageIndices = &window->image_index;
    presentation_information.pWaitSemaphores = &render_finished_semaphore;
    presentation_information.waitSemaphoreCount = 1;
    presentation_information.pResults = NULL;

    result = vkQueuePresentKHR(context->present_queue, &presentation_information);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to present image: %s", window->frame_index, string_VkResult(result));
        return 0;
    }

    return 1;
}

/*********************************************************************************************************************/
// Window Rendering Function
/*********************************************************************************************************************/

void window_render_something(struct Window *const window)
{
    VkCommandBuffer const command_buffer = window->command_buffers[window->frame_index];

    VkRenderPassBeginInfo render_pass_beginning_information = {};
    render_pass_beginning_information.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_beginning_information.renderPass = window->render_pass;
    render_pass_beginning_information.framebuffer = window->framebuffers[window->frame_index];
    render_pass_beginning_information.renderArea.extent = window->window_extent;
    render_pass_beginning_information.renderArea.offset.x = 0;
    render_pass_beginning_information.renderArea.offset.y = 0;

    VkClearValue clear_values[2];
    clear_values[0].color.float32[0] = 0.0f;
    clear_values[0].color.float32[1] = 0.0f;
    clear_values[0].color.float32[2] = 0.0f;
    clear_values[0].color.float32[3] = 1.0f;
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;

    render_pass_beginning_information.pClearValues = clear_values;
    render_pass_beginning_information.clearValueCount = 2;

    vkCmdBeginRenderPass(command_buffer, &render_pass_beginning_information, VK_SUBPASS_CONTENTS_INLINE);

    const float radians = acos(-1.0) / 180.0f;

    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    const float current_time = time.tv_sec + time.tv_nsec / 1e9;

    const float rotation_axis[3] = {0.0f, 0.0f, 1.0f};
    matrix_4x4_rotate(window->uniform_buffer_object.model, current_time * radians * 90.0f, rotation_axis);

    const float pov[3] = {2.0f, 2.0f, 2.0f};
    const float center[3] = {0.0f, 0.0f, 0.0f};
    const float up[3] = {0.0f, 0.0f, 1.0f};
    matrix_4x4_view(window->uniform_buffer_object.view, pov, center, up);

    float aspect_ratio = window->window_extent.width / (float)window->window_extent.height;
    matrix_4x4_perspective(window->uniform_buffer_object.projection, radians * 45.0f, aspect_ratio, 0.1f, 10.0f);

    window->uniform_buffer_object.projection[1][1] *= -1.0f;

    void *uniform_buffer_memory = window->uniform_buffers_mapped[window->frame_index];
    memcpy(uniform_buffer_memory, &window->uniform_buffer_object, sizeof(window->uniform_buffer_object));

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, window->pipeline);

    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &window->vertex_buffer, offsets);

    vkCmdBindIndexBuffer(command_buffer, window->index_buffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets
    (
        command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, window->pipeline_layout, 0, 1,
        &window->descriptor_sets[window->frame_index], 0, NULL
    );

    vkCmdDrawIndexed(command_buffer, window->index_count, 1, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);
}

/*********************************************************************************************************************/
// Window Destruction Function
/*********************************************************************************************************************/

void window_destroy(struct Window *const window)
{
    VkResult result;

    if (window == NULL)
    {
        return;
    }

    struct Context const *context = window->context;
    VkInstance const vulkan_instance = context->vulkan_instance;
    VkDevice const logical_device = context->logical_device;

    VkCommandBuffer *command_buffers = window->command_buffers;
    uint32_t const frame_index = window->frame_index;

    result = vkEndCommandBuffer(command_buffers[frame_index]);
    if (result != VK_SUCCESS)
    {
        log_warning("Failed to end last command buffer #%d: %s", frame_index, string_VkResult(result));
    }

    vkDeviceWaitIdle(logical_device);

    for (uint32_t frame_index = 0; frame_index < window->maximum_frames_in_flight; ++frame_index)
    {
        vkDestroyFence(logical_device, window->in_flight_fences[frame_index], NULL);
        vkDestroySemaphore(logical_device, window->render_finished_semaphores[frame_index], NULL);
        vkDestroySemaphore(logical_device, window->image_available_semaphores[frame_index], NULL);
        vkDestroyFramebuffer(logical_device, window->framebuffers[frame_index], NULL);
        vkDestroyBuffer(logical_device, window->uniform_buffers[frame_index], NULL);
        vkFreeMemory(logical_device, window->uniform_buffers_memory[frame_index], NULL);
        free(window->uniform_buffers_mapped[frame_index]);
    }

    free(window->in_flight_fences);
    free(window->render_finished_semaphores);
    free(window->image_available_semaphores);
    free(window->framebuffers);
    free(window->uniform_buffers_mapped);
    free(window->uniform_buffers_memory);
    free(window->uniform_buffers);

    free(window->descriptor_sets);
    vkDestroyDescriptorPool(logical_device, window->descriptor_pool, NULL);
    vkDestroyDescriptorSetLayout(logical_device, window->descriptor_set_layout, NULL);

    vkDestroyBuffer(logical_device, window->index_buffer, NULL);
    vkFreeMemory(logical_device, window->index_buffer_memory, NULL);

    vkDestroyBuffer(logical_device, window->vertex_buffer, NULL);
    vkFreeMemory(logical_device, window->vertex_buffer_memory, NULL);

    free(window->indices);
    free(window->vertices);

    vkDestroySampler(logical_device, window->texture_sampler, NULL);
    vkDestroyImageView(logical_device, window->texture_image_view, NULL);
    vkDestroyImage(logical_device, window->texture_image, NULL);
    vkFreeMemory(logical_device, window->texture_image_memory, NULL);

    vkDestroyImageView(logical_device, window->depth_image_view, NULL);
    vkDestroyImage(logical_device, window->depth_image, NULL);
    vkFreeMemory(logical_device, window->depth_image_memory, NULL);

    vkDestroyCommandPool(logical_device, window->command_pool, NULL);
    free(command_buffers);

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

/*********************************************************************************************************************/
// Memory Allocation Function
/*********************************************************************************************************************/

static inline uint8_t memory_allocate
(
    const struct Window *const window, const VkMemoryRequirements memory_requirements,
    const VkMemoryPropertyFlags memory_properties, VkDeviceMemory *memory
)
{
    VkResult result;

    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(window->context->physical_device, &physical_device_memory_properties);

    VkMemoryAllocateInfo memory_allocation_information = {};
    memory_allocation_information.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocation_information.allocationSize = memory_requirements.size;
    memory_allocation_information.memoryTypeIndex = UINT32_MAX;

    for
    (
        uint32_t physical_device_memory_type_index = 0;
        physical_device_memory_type_index < physical_device_memory_properties.memoryTypeCount;
        ++physical_device_memory_type_index
    )
    {
        const VkMemoryType physical_device_memory_type =
        (
            physical_device_memory_properties.memoryTypes[physical_device_memory_type_index]
        );

        if
        (
            (memory_requirements.memoryTypeBits & (1 << physical_device_memory_type_index)) &&
            (physical_device_memory_type.propertyFlags & memory_properties) == memory_properties
        )
        {
            memory_allocation_information.memoryTypeIndex = physical_device_memory_type_index;
            break;
        }
    }

    if (memory_allocation_information.memoryTypeIndex == UINT32_MAX)
    {
        log_error("Failed to find suitable physical device memory type for memory allocation");
        return 0;
    }

    result = vkAllocateMemory(window->context->logical_device, &memory_allocation_information, NULL, memory);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to allocate memory: %s", string_VkResult(result));
        return 0;
    }

    return 1;
}

/*********************************************************************************************************************/
// Single Time Command Buffer Functions
/*********************************************************************************************************************/

static inline uint8_t command_start(const struct Window *const window, VkCommandBuffer *command_buffer)
{
    VkResult result;

    VkCommandBufferAllocateInfo command_buffer_allocation_information = {};
    command_buffer_allocation_information.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocation_information.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocation_information.commandPool = window->command_pool;
    command_buffer_allocation_information.commandBufferCount = 1;

    result = vkAllocateCommandBuffers
    (
        window->context->logical_device, &command_buffer_allocation_information, command_buffer
    );

    if (result != VK_SUCCESS)
    {
        log_error("Failed to allocate single time command buffer: %s", string_VkResult(result));
        return 0;
    }

    VkCommandBufferBeginInfo command_buffer_beginning_information = {};
    command_buffer_beginning_information.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_beginning_information.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(*command_buffer, &command_buffer_beginning_information);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to begin single time command buffer: %s", string_VkResult(result));
        vkFreeCommandBuffers(window->context->logical_device, window->command_pool, 1, command_buffer);
        return 0;
    }

    return 1;
}

static inline uint8_t command_finish(const struct Window *const window, VkCommandBuffer command_buffer)
{
    VkResult result;

    result = vkEndCommandBuffer(command_buffer);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to end single time command buffer: %s", string_VkResult(result));
        vkFreeCommandBuffers(window->context->logical_device, window->command_pool, 1, &command_buffer);
        return 0;
    }

    VkSubmitInfo submission_information = {};
    submission_information.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submission_information.commandBufferCount = 1;
    submission_information.pCommandBuffers = &command_buffer;

    result = vkQueueSubmit(window->context->graphics_queue, 1, &submission_information, VK_NULL_HANDLE);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to submit single time command buffer to graphics queue: %s", string_VkResult(result));
        vkFreeCommandBuffers(window->context->logical_device, window->command_pool, 1, &command_buffer);
        return 0;
    }

    result = vkQueueWaitIdle(window->context->graphics_queue);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to wait for graphics queue: %s", string_VkResult(result));
        vkFreeCommandBuffers(window->context->logical_device, window->command_pool, 1, &command_buffer);
        return 0;
    }

    vkFreeCommandBuffers(window->context->logical_device, window->command_pool, 1, &command_buffer);

    return 1;
}

/*********************************************************************************************************************/
// Buffer Creation Function
/*********************************************************************************************************************/

static inline uint8_t buffer_create
(
    const struct Window *const window, const VkDeviceSize buffer_size, const VkBufferUsageFlags buffer_usages,
    const VkMemoryPropertyFlags memory_properties, VkBuffer *buffer, VkDeviceMemory *buffer_memory
)
{
    VkResult result;

    VkBufferCreateInfo buffer_creation_information = {};
    buffer_creation_information.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_creation_information.size = buffer_size;
    buffer_creation_information.usage = buffer_usages;
    buffer_creation_information.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateBuffer(window->context->logical_device, &buffer_creation_information, NULL, buffer);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to create buffer: %s", string_VkResult(result));
        return 0;
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(window->context->logical_device, *buffer, &memory_requirements);
    if (memory_allocate(window, memory_requirements, memory_properties, buffer_memory) == 0)
    {
        log_error("Failed to allocate memory for buffer: %s", string_VkResult(result));
        vkDestroyBuffer(window->context->logical_device, *buffer, NULL);
        return 0;
    }

    result = vkBindBufferMemory(window->context->logical_device, *buffer, *buffer_memory, 0);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to bind buffer memory: %s", string_VkResult(result));
        vkDestroyBuffer(window->context->logical_device, *buffer, NULL);
        vkFreeMemory(window->context->logical_device, *buffer_memory, NULL);
        return 0;
    }

    return 1;
}

/*********************************************************************************************************************/
// Buffer Copy Function
/*********************************************************************************************************************/

static inline uint8_t buffer_copy
(
    const struct Window *const window, VkBuffer source_buffer, VkBuffer destination_buffer,
    const VkDeviceSize buffer_size
)
{
    VkCommandBuffer command_buffer;
    if (command_start(window, &command_buffer) == 0)
    {
        log_error("Failed to start single time command buffer");
        return 0;
    }

    VkBufferCopy buffer_copy_region = {};
    buffer_copy_region.size = buffer_size;
    vkCmdCopyBuffer(command_buffer, source_buffer, destination_buffer, 1, &buffer_copy_region);

    if (command_finish(window, command_buffer) == 0)
    {
        log_error("Failed to finish single time command buffer");
        return 0;
    }

    return 1;
}

/*********************************************************************************************************************/
// PNG Image to Texture Image and Texture Image Memory Function
/*********************************************************************************************************************/

static inline uint8_t image_layout_transition
(
    const struct Window *const window, VkImage image, const VkFormat format, const VkImageLayout previous_image_layout,
    const VkImageLayout next_image_layout
);

static inline uint8_t buffer_copy_to_image
(
    const struct Window *const window, VkBuffer buffer, VkImage image, const uint32_t width, const uint32_t height
);

static inline uint8_t image_load
(
    const struct Window *const window, const char *const file_path, VkImage *texture_image,
    VkDeviceMemory *texture_image_memory
)
{
    VkResult result;

    struct Image *const image = image_create(file_path);
    if (image == NULL)
    {
        log_error("Failed to create image");
        return 0;
    }

    VkDeviceSize image_size = 4 * image->width * image->height;

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;

    if
    (
        buffer_create
        (
            window, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer,
            &staging_buffer_memory
        ) == 0
    )
    {
        log_error("Failed to create staging buffer for PNG image");
        image_destroy(image);
        return 0;
    }

    void *staging_buffer_data;
    result = vkMapMemory
    (
        window->context->logical_device, staging_buffer_memory, 0, image_size, 0, &staging_buffer_data
    );

    if (result != VK_SUCCESS)
    {
        log_error("Failed to map staging buffer memory: %s", string_VkResult(result));
        vkDestroyBuffer(window->context->logical_device, staging_buffer, NULL);
        image_destroy(image);
        return 0;
    }

    for (uint32_t row = 0; row < image->height; ++row)
    {
        memcpy((uint8_t *)staging_buffer_data + row * image->width * 4, image->data[row], image->width * 4);
    }

    vkUnmapMemory(window->context->logical_device, staging_buffer_memory);

    VkImageCreateInfo image_creation_information = {};
    image_creation_information.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_creation_information.imageType = VK_IMAGE_TYPE_2D;
    image_creation_information.extent.width = (uint32_t)image->width;
    image_creation_information.extent.height = (uint32_t)image->height;
    image_creation_information.extent.depth = 1;
    image_creation_information.mipLevels = 1;
    image_creation_information.arrayLayers = 1;
    image_creation_information.format = VK_FORMAT_R8G8B8A8_SRGB;
    image_creation_information.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_creation_information.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_creation_information.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_creation_information.samples = VK_SAMPLE_COUNT_1_BIT;
    image_creation_information.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_creation_information.flags = 0;

    result = vkCreateImage(window->context->logical_device, &image_creation_information, NULL, texture_image);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to create Vulkan image: %s", string_VkResult(result));
        vkDestroyBuffer(window->context->logical_device, staging_buffer, NULL);
        vkFreeMemory(window->context->logical_device, staging_buffer_memory, NULL);
        image_destroy(image);
        return 0;
    }

    VkMemoryRequirements image_memory_requirements;
    vkGetImageMemoryRequirements(window->context->logical_device, *texture_image, &image_memory_requirements);

    if
    (
        memory_allocate
        (
            window, image_memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture_image_memory
        ) == 0
    )
    {
        log_error("Failed to allocate memory for image");
        vkDestroyBuffer(window->context->logical_device, staging_buffer, NULL);
        vkFreeMemory(window->context->logical_device, staging_buffer_memory, NULL);
        image_destroy(image);
        return 0;
    }

    result = vkBindImageMemory(window->context->logical_device, *texture_image, *texture_image_memory, 0);
    if (result != VK_SUCCESS)
    {
        log_error("Failed to bind image memory: %s", string_VkResult(result));
        vkDestroyBuffer(window->context->logical_device, staging_buffer, NULL);
        vkFreeMemory(window->context->logical_device, staging_buffer_memory, NULL);
        image_destroy(image);
        return 0;
    }

    if
    (
        image_layout_transition
        (
            window, *texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        ) == 0
    )
    {
        log_error("Failed to transition image layout");
        vkDestroyBuffer(window->context->logical_device, staging_buffer, NULL);
        vkFreeMemory(window->context->logical_device, staging_buffer_memory, NULL);
        image_destroy(image);
        return 0;
    }

    if
    (
        buffer_copy_to_image
        (
            window, staging_buffer, *texture_image, (uint32_t)image->width, (uint32_t)image->height
        ) == 0
    )
    {
        log_error("Failed to copy buffer to image");
        vkDestroyBuffer(window->context->logical_device, staging_buffer, NULL);
        vkFreeMemory(window->context->logical_device, staging_buffer_memory, NULL);
        image_destroy(image);
        return 0;
    }

    if
    (
        image_layout_transition
        (
            window, *texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        ) == 0
    )
    {
        log_error("Failed to transition image layout");
        vkDestroyBuffer(window->context->logical_device, staging_buffer, NULL);
        vkFreeMemory(window->context->logical_device, staging_buffer_memory, NULL);
        image_destroy(image);
        return 0;
    }

    vkDestroyBuffer(window->context->logical_device, staging_buffer, NULL);
    vkFreeMemory(window->context->logical_device, staging_buffer_memory, NULL);
    image_destroy(image);

    return 1;
}

static inline uint8_t image_layout_transition
(
    const struct Window *const window, VkImage image, const VkFormat format, const VkImageLayout previous_image_layout,
    const VkImageLayout next_image_layout
)
{
    VkCommandBuffer command_buffer;
    if (command_start(window, &command_buffer) == 0)
    {
        log_error("Failed to start single time command buffer");
        return 0;
    }

    VkImageMemoryBarrier image_memory_barrier = {};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.oldLayout = previous_image_layout;
    image_memory_barrier.newLayout = next_image_layout;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = image;
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags source_stages;
    VkPipelineStageFlags destination_stages;

    if
    (
        previous_image_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        next_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    )
    {
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if
    (
        previous_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        next_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    )
    {
        image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stages = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        log_error
        (
            "Unsupported image layout transition from '%s' to '%s'", string_VkImageLayout(previous_image_layout),
            string_VkImageLayout(next_image_layout)
        );

        command_finish(window, command_buffer);
        return 0;
    }

    vkCmdPipelineBarrier
    (
        command_buffer, source_stages, destination_stages, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier
    );

    if (command_finish(window, command_buffer) == 0)
    {
        log_error("Failed to finish single time command buffer");
        return 0;
    }

    return 1;
}

static inline uint8_t buffer_copy_to_image
(
    const struct Window *const window, VkBuffer buffer, VkImage image, const uint32_t width, const uint32_t height
)
{
    VkCommandBuffer command_buffer;
    if (command_start(window, &command_buffer) == 0)
    {
        log_error("Failed to start single time command buffer");
        return 0;
    }

    VkBufferImageCopy buffer_image_copy_region = {};
    buffer_image_copy_region.bufferOffset = 0;
    buffer_image_copy_region.bufferRowLength = 0;
    buffer_image_copy_region.bufferImageHeight = 0;
    buffer_image_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_image_copy_region.imageSubresource.mipLevel = 0;
    buffer_image_copy_region.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy_region.imageSubresource.layerCount = 1;
    buffer_image_copy_region.imageOffset.x = 0;
    buffer_image_copy_region.imageOffset.y = 0;
    buffer_image_copy_region.imageOffset.z = 0;
    buffer_image_copy_region.imageExtent.width = width;
    buffer_image_copy_region.imageExtent.height = height;
    buffer_image_copy_region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage
    (
        command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy_region
    );

    if (command_finish(window, command_buffer) == 0)
    {
        log_error("Failed to finish single time command buffer");
        return 0;
    }

    return 1;
}