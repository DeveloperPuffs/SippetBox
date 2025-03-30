#include <stdlib.h>
#include <string.h>
#include <math.h>
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

#define VERTEX_SIZE (13 * sizeof(float))
#define INDEX_SIZE (sizeof(uint32_t))

struct Application {
  const char *name;
  VkInstance vulkanInstance;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkPhysicalDevice physicalDevice;
  uint32_t graphicsQueueFamilyIndex;
  uint32_t presentQueueFamilyIndex;
  VkDevice logicalDevice;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
};

struct Window {
  const struct Application *application;
  SDL_Window *sdlWindowHandle;
  VkExtent2D windowExtent;
  VkSurfaceKHR surface;
  VkSurfaceFormatKHR surfaceFormat;
  VkSwapchainKHR swapchain;
  uint32_t imageCount;
  VkImage *images;
  VkImageView *imageViews;
  VkRenderPass renderPass;
  size_t shaderModuleCount;
  VkShaderModule *shaderModules;
  VkDescriptorSetLayout descriptorSetLayout;
  VkPipelineLayout pipelineLayout;
  VkPipeline pipeline;
  uint32_t maximumFramesInFlight;
  VkCommandPool commandPool;
  VkCommandBuffer *commandBuffers;
  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;
  VkImage textureImage;
  VkDeviceMemory textureImageMemory;
  VkImageView textureImageView;
  VkSampler textureSampler;
  size_t indexCount;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;
  VkBuffer *uniformBuffers;
  VkDeviceMemory *uniformBuffersMemory;
  void **uniformBuffersMapped;
  VkDescriptorPool descriptorPool;
  VkDescriptorSet *descriptorSets;
  VkFramebuffer *framebuffers;
  VkSemaphore *imageAvailableSemaphores;
  VkSemaphore *renderFinishedSemaphores;
  VkFence *inFlightFences;
  uint32_t imageIndex;
  uint32_t frameIndex;
  bool pendingFrame;
  struct UniformBufferObject {
    float model[4][4];
    float view[4][4];
    float projection[4][4];
  } uniformBufferObject;
};

static inline VkBool32 callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *callbackData, void *userData);
static uint32_t ratePhysicalDeviceProperties(const VkPhysicalDevice physicalDevice);
static uint32_t ratePhysicalDeviceFeatures(const VkPhysicalDevice physicalDevice);
static uint32_t ratePhysicalDeviceMemory(const VkPhysicalDevice physicalDevice);
static uint32_t ratePhysicalDeviceQueueFamilies(const VkPhysicalDevice physicalDevice);
static uint32_t ratePhysicalDeviceExtensions(const VkPhysicalDevice physicalDevice);

struct Application *createApplication(const char *name) {
  VkResult result;

  struct Application *const application = calloc(1, sizeof(struct Application));
  if (!application) return NULL;

  application->name = name;

  VkInstanceCreateInfo instanceCreationInformation = {0};
  instanceCreationInformation.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreationInformation.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

  VkApplicationInfo applicationInformation = {0};
  applicationInformation.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  applicationInformation.pApplicationName = name;
  applicationInformation.pEngineName = "SippetBox";
  applicationInformation.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  applicationInformation.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  applicationInformation.apiVersion = VK_API_VERSION_1_0;
  instanceCreationInformation.pApplicationInfo = &applicationInformation;

#ifdef NDEBUG
  const char *const instanceLayers[] = {};
#else
  /*
    "VK_LAYER_KHRONOS_validation"
    For reporting error messages about potential API misuse and, performance issues, or invalid synchronization
  */
  const char *const instanceLayers[] = {
    "VK_LAYER_KHRONOS_validation"
  };
#endif

  const size_t instanceLayerCount = sizeof(instanceLayers) / sizeof(instanceLayers[0]);
  instanceCreationInformation.enabledLayerCount = instanceLayerCount;
  instanceCreationInformation.ppEnabledLayerNames = instanceLayers;

  Uint32 sdlInstanceExtensionCount = 0;
  const char *const *const sdlInstanceExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlInstanceExtensionCount);
  if (!sdlInstanceExtensions) return (logError("Failed to query SDL Vulkan instance extensions: %s", SDL_GetError()), destroyApplication(application), NULL);

#ifdef PLATFORM_APPLE
  /*
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
    For running Vulkan on a macOS environment via MoltenVK

    VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    For setting up a debug messenger associated with the Vulkan instance
  */
  const char *const additionalInstanceExtensions[] = {
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME
  };
#else
  /*
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    For setting up a debug messenger associated with the Vulkan instance
  */
  const char *const additionalInstanceExtensions[] = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME
  };
#endif

  const size_t additionalInstanceExtensionCount = sizeof(additionalInstanceExtensions) / sizeof(additionalInstanceExtensions[0]);
  const size_t instanceExtensionCount = (size_t)sdlInstanceExtensionCount + additionalInstanceExtensionCount;
  const char **const instanceExtensions = malloc(instanceExtensionCount * sizeof(const char *));
  if (!instanceExtensions) return (destroyApplication(application), NULL);
  memcpy(instanceExtensions, sdlInstanceExtensions, (size_t)sdlInstanceExtensionCount * sizeof(const char *));
  memcpy(instanceExtensions + (size_t)sdlInstanceExtensionCount, additionalInstanceExtensions, additionalInstanceExtensionCount * sizeof(const char *));
  instanceCreationInformation.enabledExtensionCount = instanceExtensionCount;
  instanceCreationInformation.ppEnabledExtensionNames = instanceExtensions;

  if ((result = vkCreateInstance(&instanceCreationInformation, NULL, &application->vulkanInstance)))
    return (logError("Failed to create Vulkan instance: %s", string_VkResult(result)), free(instanceExtensions), destroyApplication(application), NULL);
  free(instanceExtensions);

  VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreationInformation = {0};
  debugMessengerCreationInformation.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debugMessengerCreationInformation.pfnUserCallback = callback;
  debugMessengerCreationInformation.pUserData = NULL;
  debugMessengerCreationInformation.messageSeverity =
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
  debugMessengerCreationInformation.messageType =
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

  PFN_vkCreateDebugUtilsMessengerEXT createDebugMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(application->vulkanInstance, "vkCreateDebugUtilsMessengerEXT");
  if (!createDebugMessenger) return (logError("Failed to query debug messenger creation function"), destroyApplication(application), NULL);
  if ((result = createDebugMessenger(application->vulkanInstance, &debugMessengerCreationInformation, NULL, &application->debugMessenger)))
    return (logError("Failed to create debug messenger: %s", string_VkResult(result)), destroyApplication(application), NULL);

  uint32_t physicalDeviceCount;
  if ((result = vkEnumeratePhysicalDevices(application->vulkanInstance, &physicalDeviceCount, NULL)))
    return (logError("Failed to query physical device count: %s", string_VkResult(result)), destroyApplication(application), NULL);
  if (!physicalDeviceCount) return (logError("No physical device found"), destroyApplication(application), NULL);

  VkPhysicalDevice *const physicalDevices = malloc(physicalDeviceCount * sizeof(VkPhysicalDevice));
  if (!physicalDevices) return (destroyApplication(application), NULL);
  if ((result = vkEnumeratePhysicalDevices(application->vulkanInstance, &physicalDeviceCount, physicalDevices)))
    return (logError("Failed to query physical devices: %s", string_VkResult(result)), destroyApplication(application), NULL);

  uint32_t bestScore = 0;
  for (uint32_t i = 0; i < physicalDeviceCount; ++i) {
    VkPhysicalDevice const physicalDevice = physicalDevices[i];
    const uint32_t score =
      ratePhysicalDeviceProperties(physicalDevice) +
      ratePhysicalDeviceFeatures(physicalDevice) +
      ratePhysicalDeviceMemory(physicalDevice) +
      ratePhysicalDeviceQueueFamilies(physicalDevice) +
      ratePhysicalDeviceExtensions(physicalDevice);

    if (score > bestScore) (application->physicalDevice = physicalDevice, bestScore = score);
  }

  free(physicalDevices);
  if (!bestScore) return (logError("Failed to find suitable physical device"), destroyApplication(application), NULL);
  logInformation("Selected physical device with score %d", bestScore);

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(application->physicalDevice, &queueFamilyCount, NULL);

  VkQueueFamilyProperties *const queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
  if (!queueFamilies) return (destroyApplication(application), NULL);
  vkGetPhysicalDeviceQueueFamilyProperties(application->physicalDevice, &queueFamilyCount, queueFamilies);

  // A window and a window surface have to be created to ensure that the present queue family supports presenting to a window surface created in this application
  SDL_Window *const windowModel = SDL_CreateWindow("", 1, 1, SDL_WINDOW_VULKAN);
  if (!windowModel) return (logError("Failed to create window model: %s", SDL_GetError()), destroyApplication(application), free(queueFamilies), NULL);

  VkSurfaceKHR surfaceModel;
  if (!SDL_Vulkan_CreateSurface(windowModel, application->vulkanInstance, NULL, &surfaceModel))
    return (logError("Failed to create surface model: %s", SDL_GetError()), destroyApplication(application), SDL_DestroyWindow(windowModel), free(queueFamilies), NULL);

  uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
  uint32_t presentQueueFamilyIndex = UINT32_MAX;
  for (uint32_t i = 0; i < queueFamilyCount; ++i) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && graphicsQueueFamilyIndex == UINT32_MAX) graphicsQueueFamilyIndex = i;

    VkBool32 supportsPresenting = VK_FALSE;
    result = vkGetPhysicalDeviceSurfaceSupportKHR(application->physicalDevice, i, surfaceModel, &supportsPresenting);
    if (result) {
      logWarning("Failed to query physical device surface queue family %d presenting support verdict: %s", i, string_VkResult(result));
      continue;
    }

    if (supportsPresenting && presentQueueFamilyIndex == UINT32_MAX) presentQueueFamilyIndex = i;
  }

  vkDestroySurfaceKHR(application->vulkanInstance, surfaceModel, NULL);
  SDL_DestroyWindow(windowModel);
  free(queueFamilies);

  if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX)
    return (logError("Failed to find suitable queue family indices"), destroyApplication(application), NULL);

  VkDeviceCreateInfo deviceCreationInformation = {};
  deviceCreationInformation.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  float queuePriority = 1.0f;
  uint32_t queueCreationInformationCount = 1;

  VkDeviceQueueCreateInfo queueCreationInformations[2] = {};
  queueCreationInformations[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreationInformations[0].queueFamilyIndex = graphicsQueueFamilyIndex;
  queueCreationInformations[0].pQueuePriorities = &queuePriority;
  queueCreationInformations[0].queueCount = 1;

  if (presentQueueFamilyIndex != graphicsQueueFamilyIndex) {
    queueCreationInformationCount++;
    queueCreationInformations[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreationInformations[1].queueFamilyIndex = presentQueueFamilyIndex;
    queueCreationInformations[1].pQueuePriorities = &queuePriority;
    queueCreationInformations[1].queueCount = 1;
  }

  deviceCreationInformation.pQueueCreateInfos = queueCreationInformations;
  deviceCreationInformation.queueCreateInfoCount = queueCreationInformationCount;

#ifdef NDEBUG
  const char *const logicalDeviceLayers[] = {};
#else
  /*
    "VK_LAYER_KHRONOS_validation"
    For reporting error messages about potential API misuse and, performance issues, or invalid synchronization
  */
  const char *const logicalDeviceLayers[] = {
    "VK_LAYER_KHRONOS_validation"
  };
#endif

  const size_t logicalDeviceLayerCount = sizeof(logicalDeviceLayers) / sizeof(logicalDeviceLayers[0]);
  deviceCreationInformation.enabledLayerCount = logicalDeviceLayerCount;
  deviceCreationInformation.ppEnabledLayerNames = logicalDeviceLayers;

  // TODO: Enable the following logical device extensions for ray tracing or path tracing:
  // VK_KHR_acceleration_structure, VK_KHR_ray_tracing_pipeline, VK_KHR_deferred_host_operations

#ifdef PLATFORM_APPLE
  /*
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
    For presenting on a window surface

    "VK_KHR_portability_subset"
    For running Vulkan on a macOS environment via MoltenVK
  */
  const char *const logicalDeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_portability_subset"
  };
#else
  /*
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
    For presenting on a window surface
  */
  const char *const logicalDeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };
#endif

  const size_t logicalDeviceExtensionCount = sizeof(logicalDeviceExtensions) / sizeof(logicalDeviceExtensions[0]);
  deviceCreationInformation.enabledExtensionCount = logicalDeviceExtensionCount;
  deviceCreationInformation.ppEnabledExtensionNames = logicalDeviceExtensions;

  if ((result = vkCreateDevice(application->physicalDevice, &deviceCreationInformation, NULL, &application->logicalDevice)))
    return (logError("Failed to create logical device: %s", string_VkResult(result)), destroyApplication(application), NULL);

  vkGetDeviceQueue(application->logicalDevice, application->graphicsQueueFamilyIndex, 0, &application->graphicsQueue);
  vkGetDeviceQueue(application->logicalDevice, application->presentQueueFamilyIndex, 0, &application->presentQueue);
  return application;
}

void destroyApplication(struct Application *const application) {
  if (!application) return;

  VkInstance const vulkanInstance = application->vulkanInstance;
  vkDestroyDevice(application->logicalDevice, NULL);

  if (vulkanInstance) {
    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanInstance, "vkDestroyDebugUtilsMessengerEXT");
    if (!destroyDebugMessenger) logWarning("Failed to query debug messenger destruction function");
    else destroyDebugMessenger(vulkanInstance, application->debugMessenger, NULL);
  }

  vkDestroyInstance(vulkanInstance, NULL);
  free(application);
}

static inline VkBool32 callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *callbackData, void *userData) {
  char typeString[64] = {0};
  uint8_t leadingComma = 0;

  if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) (snprintf(typeString, sizeof(typeString), "GENERAL"), leadingComma = 1);

  if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
    if (!leadingComma) (snprintf(typeString, sizeof(typeString), "VALIDATION"), leadingComma = 1);
    else snprintf(typeString + strlen(typeString), sizeof(typeString) - strlen(typeString), ", VALIDATION");
  }

  if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
    if (!leadingComma) (snprintf(typeString, sizeof(typeString), "PERFORMANCE"), leadingComma = 1);
    else snprintf(typeString + strlen(typeString), sizeof(typeString) - strlen(typeString), ", PERFORMANCE");
  }

  if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
    if (!leadingComma) (snprintf(typeString, sizeof(typeString), "DEVIVE_ADDRESS_BINDING"), leadingComma = 1);
    else snprintf(typeString + strlen(typeString), sizeof(typeString) - strlen(typeString), ", DEVIVE_ADDRESS_BINDING");
  }

  if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) logError("%s - %s", typeString, callbackData->pMessage);
  if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) logWarning("%s - %s", typeString, callbackData->pMessage);
  if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) logInformation("%s - %s", typeString, callbackData->pMessage);
  if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) logDebug("%s - %s", typeString, callbackData->pMessage);
  return VK_FALSE;
}

static uint32_t ratePhysicalDeviceProperties(const VkPhysicalDevice physicalDevice) {
  uint32_t score = 0;

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

  // It would be nice to have the physical device support Vulkan version 1.3 at least
  const uint32_t apiVersion = deviceProperties.apiVersion;
  const uint32_t majorVersion = VK_VERSION_MAJOR(apiVersion);
  const uint32_t minorVersion = VK_VERSION_MINOR(apiVersion);
  if (majorVersion >= 2 || (majorVersion == 1 && minorVersion >= 3)) score += 500;
  if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
  return score + deviceProperties.limits.maxImageDimension2D;;
}

static uint32_t ratePhysicalDeviceFeatures(const VkPhysicalDevice physicalDevice) {
  uint32_t score = 0;

  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
  if (deviceFeatures.samplerAnisotropy) score += 500;
  if (deviceFeatures.geometryShader) score += 200;
  return score;
}

static uint32_t ratePhysicalDeviceMemory(const VkPhysicalDevice physicalDevice) {
  uint32_t score = 0;

  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
  for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++) {
    // The total memory will be converted to megabytes
    const VkMemoryHeap memoryHeap = memoryProperties.memoryHeaps[i];
    if (memoryHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) score += memoryHeap.size / (1024 * 1024);
  }

  return score / 10;
}

static uint32_t ratePhysicalDeviceQueueFamilies(const VkPhysicalDevice physicalDevice) {
  uint32_t score = 0;

  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
  VkQueueFamilyProperties *const queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
  if (!queueFamilies) return score;

  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);
  for (uint32_t i = 0; i < queueFamilyCount; ++i) {
    const VkQueueFlags queueFamilyFlags = queueFamilies[i].queueFlags;
    if (queueFamilyFlags & VK_QUEUE_GRAPHICS_BIT) score += 500;
    if (queueFamilyFlags & VK_QUEUE_COMPUTE_BIT) score += 300;
    if (queueFamilyFlags & VK_QUEUE_TRANSFER_BIT) score += 200;
  }

  free(queueFamilies);
  return score;
}

static uint32_t ratePhysicalDeviceExtensions(const VkPhysicalDevice physicalDevice) {
  VkResult result;
  uint32_t score = 0;

  uint32_t extensionCount;
  if ((result = vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, NULL)))
    return (logWarning("Failed to query physical device extension count: %s", string_VkResult(result)), score);

  VkExtensionProperties *const extensions = malloc(extensionCount * sizeof(VkExtensionProperties));
  if (!extensions) return score;

  if ((result = vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &extensionCount, extensions)))
    return (logWarning("Failed to query physical device extensions: %s", string_VkResult(result)), free(extensions), score);

  for (uint32_t i = 0; i < extensionCount; ++i) {
    if (!strcmp(extensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
      score += 1000;
      break;
    }
  }

  free(extensions);
  return score;
}

static inline bool windowBeginFrame(struct Window *const window);
static inline bool windowEndFrame(const struct Window *const window);
static inline bool memoryAllocate(const struct Window *const window, const VkMemoryRequirements requirements, const VkMemoryPropertyFlags properties, VkDeviceMemory *memory);
static inline bool commandStart(const struct Window *const window, VkCommandBuffer *commandBuffer);
static inline bool commandFinish(const struct Window *const window, VkCommandBuffer commandBuffer);
static inline bool bufferCreate(const struct Window *const window, const VkDeviceSize size, const VkBufferUsageFlags usages, const VkMemoryPropertyFlags memoryProperties, VkBuffer *buffer, VkDeviceMemory *memory);
static inline bool bufferCopy(const struct Window *const window, VkBuffer source, VkBuffer destination, const VkDeviceSize size);
static inline bool textureLoad(const struct Window *const window, const char *const path, VkImage *texture, VkDeviceMemory *memory);

struct Window *createWindow(const struct Application *application, const char *const title, const uint32_t width, const uint32_t height) {
  if (!application) return NULL;
  VkResult result;

  struct Window *const window = calloc(1, sizeof(struct Window));
  if (!window) return NULL;

  window->application = application;
  window->windowExtent.width = width;
  window->windowExtent.height = height;
  VkInstance const vulkanInstance = application->vulkanInstance;
  VkPhysicalDevice const physicalDevice = application->physicalDevice;
  VkDevice const logicalDevice = application->logicalDevice;

  window->sdlWindowHandle = SDL_CreateWindow(title, (int)width, (int)height, SDL_WINDOW_VULKAN);
  if (!window->sdlWindowHandle) return (logError("Failed to create SDL window: %s", SDL_GetError()), destroyWindow(window), NULL);

  if (!SDL_Vulkan_CreateSurface(window->sdlWindowHandle, vulkanInstance, NULL, &window->surface))
    return (logError("Failed to create window surface: %s", SDL_GetError()), destroyWindow(window), NULL);

  uint32_t surfaceFormatCount;
  if ((result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, window->surface, &surfaceFormatCount, NULL)))
    return (logError("Failed to query surface format count: %s", string_VkResult(result)), destroyWindow(window), NULL);
  if (!surfaceFormatCount) return (logError("No surface format found"), destroyWindow(window), NULL);

  VkSurfaceFormatKHR *const surfaceFormats = malloc(surfaceFormatCount * sizeof(VkSurfaceFormatKHR));
  if (!surfaceFormats) return (destroyWindow(window), NULL);
  if ((result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, window->surface, &surfaceFormatCount, surfaceFormats)))
    return (logError("Failed to query surface formats: %s", string_VkResult(result)), destroyWindow(window), free(surfaceFormats), NULL);

  uint32_t surfaceFormatIndex = UINT32_MAX;
  for (uint32_t i = 0; i < surfaceFormatCount; ++i) {
    const VkSurfaceFormatKHR surfaceFormat = surfaceFormats[i];
    if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      surfaceFormatIndex = i;
      break;
    }
  }

  if (surfaceFormatIndex == UINT32_MAX) return (logError("Failed to find suitable surface format"), destroyWindow(window), free(surfaceFormats), NULL);
  window->surfaceFormat = surfaceFormats[surfaceFormatIndex];
  free(surfaceFormats);

  VkSwapchainCreateInfoKHR swapchainCreationInformation = {};
  swapchainCreationInformation.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainCreationInformation.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainCreationInformation.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainCreationInformation.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  swapchainCreationInformation.imageArrayLayers = 1;
  swapchainCreationInformation.clipped = VK_TRUE;
  swapchainCreationInformation.oldSwapchain = VK_NULL_HANDLE;
  swapchainCreationInformation.surface = window->surface;
  swapchainCreationInformation.imageFormat = window->surfaceFormat.format;
  swapchainCreationInformation.imageColorSpace = window->surfaceFormat.colorSpace;
  swapchainCreationInformation.imageExtent = window->windowExtent;

  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  if ((result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, window->surface, &surfaceCapabilities)))
    return (logError("Failed to query physical device surface capabilities: %s", string_VkResult(result)), destroyWindow(window), NULL);

  swapchainCreationInformation.minImageCount = surfaceCapabilities.minImageCount;
  swapchainCreationInformation.preTransform = surfaceCapabilities.currentTransform;

  if ((result = vkCreateSwapchainKHR(logicalDevice, &swapchainCreationInformation, NULL, &window->swapchain)))
    return (logError("Failed to create swapchain: %s", string_VkResult(result)), destroyWindow(window), NULL);

  if ((result = vkGetSwapchainImagesKHR(logicalDevice, window->swapchain, &window->imageCount, NULL)))
    return (logError("Failed to query swapchain image count: %d", string_VkResult(result)), destroyWindow(window), NULL);
  if (!window->imageCount) return (logError("No images found in swapchain"), destroyWindow(window), NULL);
  if (!(window->images = calloc(window->imageCount, sizeof(VkImage)))) return (destroyWindow(window), NULL);
  if ((result = vkGetSwapchainImagesKHR(logicalDevice, window->swapchain, &window->imageCount, window->images)))
    return (logError("Failed to query swapchain images: %d", string_VkResult(result)), destroyWindow(window), NULL);

  if (!(window->imageViews = calloc(window->imageCount, sizeof(VkImageView)))) return (destroyWindow(window), NULL);
  for (uint32_t i = 0; i < window->imageCount; ++i) {
    VkImageViewCreateInfo imageViewCreationInformation = {};
    imageViewCreationInformation.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreationInformation.image = window->images[i];
    imageViewCreationInformation.format = window->surfaceFormat.format;
    imageViewCreationInformation.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreationInformation.subresourceRange.baseMipLevel = 0;
    imageViewCreationInformation.subresourceRange.levelCount = 1;
    imageViewCreationInformation.subresourceRange.baseArrayLayer = 0;
    imageViewCreationInformation.subresourceRange.layerCount = 1;
    imageViewCreationInformation.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreationInformation.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreationInformation.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreationInformation.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreationInformation.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if ((result = vkCreateImageView(logicalDevice, &imageViewCreationInformation, NULL, &window->imageViews[i])))
      return (logError("Failed to create image view %d: %s", i, string_VkResult(result)), destroyWindow(window), NULL);
  }

  VkAttachmentDescription colorAttachmentDescription = {};
  colorAttachmentDescription.format = window->surfaceFormat.format;
  colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentReference = {};
  colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachmentReference.attachment = 0;

  const VkFormat depthFormats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
  const size_t depthFormatCount = sizeof(depthFormats) / sizeof(depthFormats[0]);

  size_t depthFormatIndex = SIZE_MAX;
  for (size_t i = 0; i < depthFormatCount; ++i) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, depthFormats[i], &formatProperties);
    if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      depthFormatIndex = i;
      break;
    }
  }

  if (depthFormatIndex == SIZE_MAX) return (logError("Failed to find supported depth format"), destroyWindow(window), NULL);
  const VkFormat depthFormat = depthFormats[depthFormatIndex];

  VkAttachmentDescription depthAttachmentDescription = {};
  depthAttachmentDescription.format = depthFormat;
  depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentReference = {};
  depthAttachmentReference.attachment = 1;
  depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpassDescription = {};
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
  subpassDescription.pColorAttachments = &colorAttachmentReference;
  subpassDescription.colorAttachmentCount = 1;

  VkSubpassDependency subpassDependency = {};
  subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  subpassDependency.dstSubpass = 0;
  subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  subpassDependency.srcAccessMask = 0;
  subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  const VkAttachmentDescription attachmentDescriptions[] = {colorAttachmentDescription, depthAttachmentDescription};
  const size_t attachmentDescriptionCount = sizeof(attachmentDescriptions) / sizeof(attachmentDescriptions[0]);

  VkRenderPassCreateInfo renderPassCreationInformation = {};
  renderPassCreationInformation.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreationInformation.attachmentCount = attachmentDescriptionCount;
  renderPassCreationInformation.pAttachments = attachmentDescriptions;
  renderPassCreationInformation.subpassCount = 1;
  renderPassCreationInformation.pSubpasses = &subpassDescription;
  renderPassCreationInformation.dependencyCount = 1;
  renderPassCreationInformation.pDependencies = &subpassDependency;

  if ((result = vkCreateRenderPass(logicalDevice, &renderPassCreationInformation, NULL, &window->renderPass)))
    return (logError("Failed to create render pass: %s", string_VkResult(result)), destroyWindow(window), NULL);

  const char *const shaderFilePaths[] = {"resources/shaders/shader.vert.spv", "resources/shaders/shader.frag.spv"};
  window->shaderModuleCount = sizeof(shaderFilePaths) / sizeof(shaderFilePaths[0]);
  if (!(window->shaderModules = calloc(window->shaderModuleCount, sizeof(VkShaderModule)))) return (destroyWindow(window), NULL);
  for (size_t shaderModuleIndex = 0; shaderModuleIndex < window->shaderModuleCount; ++shaderModuleIndex) {
    const char *const shaderFilePath = shaderFilePaths[shaderModuleIndex];
    struct Shader *const shader = createShader(shaderFilePath);
    if (!shader) return (logError("Failed to create shader: %s", shaderFilePath), destroyWindow(window), NULL);

    VkShaderModuleCreateInfo shaderModuleCreationInformation = {};
    shaderModuleCreationInformation.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreationInformation.codeSize = shader->size;
    shaderModuleCreationInformation.pCode = shader->data;

    if ((result = vkCreateShaderModule(logicalDevice, &shaderModuleCreationInformation, NULL, &window->shaderModules[shaderModuleIndex])))
      return (logError("Failed to create shader module: %s", string_VkResult(result)), destroyWindow(window), destroyShader(shader), NULL);
    destroyShader(shader);
  }

  VkDescriptorSetLayoutBinding uniformBufferObjectDescriptorSetLayoutBinding = {};
  uniformBufferObjectDescriptorSetLayoutBinding.binding = 0;
  uniformBufferObjectDescriptorSetLayoutBinding.descriptorCount = 1;
  uniformBufferObjectDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uniformBufferObjectDescriptorSetLayoutBinding.pImmutableSamplers = NULL;
  uniformBufferObjectDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutBinding samplerDescriptorSetLayoutBinding = {};
  samplerDescriptorSetLayoutBinding.binding = 1;
  samplerDescriptorSetLayoutBinding.descriptorCount = 1;
  samplerDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerDescriptorSetLayoutBinding.pImmutableSamplers = NULL;
  samplerDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  const VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[] = {
    uniformBufferObjectDescriptorSetLayoutBinding,
    samplerDescriptorSetLayoutBinding
  };

  const size_t descriptorSetLayoutBindingCount = sizeof(descriptorSetLayoutBindings) / sizeof(descriptorSetLayoutBindings[0]);

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreationInformation = {};
  descriptorSetLayoutCreationInformation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreationInformation.pBindings = descriptorSetLayoutBindings;
  descriptorSetLayoutCreationInformation.bindingCount = descriptorSetLayoutBindingCount;

  if ((result = vkCreateDescriptorSetLayout(logicalDevice, &descriptorSetLayoutCreationInformation, NULL, &window->descriptorSetLayout)))
    return (logError("Failed to create descriptor set layout: %s", string_VkResult(result)), destroyWindow(window), NULL);

  VkPipelineLayoutCreateInfo pipelineLayoutCreationInformation = {};
  pipelineLayoutCreationInformation.sType  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreationInformation.pSetLayouts = &window->descriptorSetLayout;
  pipelineLayoutCreationInformation.setLayoutCount = 1;
  pipelineLayoutCreationInformation.pPushConstantRanges = NULL;
  pipelineLayoutCreationInformation.pushConstantRangeCount = 0;

  if ((result = vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreationInformation, NULL, &window->pipelineLayout)))
    return (logError("Failed to create pipeline layout: %s", string_VkResult(result)), destroyWindow(window), NULL);

  VkPipelineShaderStageCreateInfo vertexShaderStageCreationInformation = {};
  vertexShaderStageCreationInformation.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertexShaderStageCreationInformation.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertexShaderStageCreationInformation.pName = "main";
  vertexShaderStageCreationInformation.module = window->shaderModules[0];

  VkPipelineShaderStageCreateInfo fragmentShaderStageCreationInformation = {};
  fragmentShaderStageCreationInformation.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragmentShaderStageCreationInformation.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragmentShaderStageCreationInformation.pName = "main";
  fragmentShaderStageCreationInformation.module = window->shaderModules[1];

  VkPipelineShaderStageCreateInfo shaderStagesCreationInformation[2];
  shaderStagesCreationInformation[0] = vertexShaderStageCreationInformation;
  shaderStagesCreationInformation[1] = fragmentShaderStageCreationInformation;

  VkPipelineVertexInputStateCreateInfo vertexInputStateCreationInformation = {};
  vertexInputStateCreationInformation.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  VkVertexInputBindingDescription vertexInputBindingDescription = {};
  vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  vertexInputBindingDescription.stride = VERTEX_SIZE;
  vertexInputBindingDescription.binding = 0;

  vertexInputStateCreationInformation.pVertexBindingDescriptions = &vertexInputBindingDescription;
  vertexInputStateCreationInformation.vertexBindingDescriptionCount = 1;

  VkVertexInputAttributeDescription vertexInputAttributeDescriptions[4] = {};

  // Represents the position of the vertex
  vertexInputAttributeDescriptions[0].location = 0;
  vertexInputAttributeDescriptions[0].binding = 0;
  vertexInputAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexInputAttributeDescriptions[0].offset = 0;

  // Represents the normal of the vertex
  vertexInputAttributeDescriptions[1].location = 1;
  vertexInputAttributeDescriptions[1].binding = 0;
  vertexInputAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexInputAttributeDescriptions[1].offset = 3 * sizeof(float);

  // Represents the texture coordinates of the vertex
  vertexInputAttributeDescriptions[2].location = 2;
  vertexInputAttributeDescriptions[2].binding = 0;
  vertexInputAttributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexInputAttributeDescriptions[2].offset = 6 * sizeof(float);

  // Represents the color of the vertex
  vertexInputAttributeDescriptions[3].location = 3;
  vertexInputAttributeDescriptions[3].binding = 0;
  vertexInputAttributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  vertexInputAttributeDescriptions[3].offset = 9 * sizeof(float);

  vertexInputStateCreationInformation.pVertexAttributeDescriptions = vertexInputAttributeDescriptions;
  vertexInputStateCreationInformation.vertexAttributeDescriptionCount = 4;

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreationInformation = {};
  inputAssemblyStateCreationInformation.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyStateCreationInformation.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyStateCreationInformation.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportStateCreationInformation = {};
  viewportStateCreationInformation.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)width;
  viewport.height = (float)height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent.width = width;
  scissor.extent.height = height;

  viewportStateCreationInformation.pViewports = &viewport;
  viewportStateCreationInformation.viewportCount = 1;
  viewportStateCreationInformation.pScissors = &scissor;
  viewportStateCreationInformation.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizationStateCreationInformation = {};
  rasterizationStateCreationInformation.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizationStateCreationInformation.depthClampEnable = VK_FALSE;
  rasterizationStateCreationInformation.rasterizerDiscardEnable = VK_FALSE;
  rasterizationStateCreationInformation.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizationStateCreationInformation.lineWidth = 1.0f;
  rasterizationStateCreationInformation.cullMode = VK_CULL_MODE_NONE;
  rasterizationStateCreationInformation.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizationStateCreationInformation.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisamplingStateCreationInformation = {};
  multisamplingStateCreationInformation.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisamplingStateCreationInformation.sampleShadingEnable = VK_FALSE;
  multisamplingStateCreationInformation.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depthStencilStateCreationInformation = {};
  depthStencilStateCreationInformation.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilStateCreationInformation.depthTestEnable = VK_TRUE;
  depthStencilStateCreationInformation.depthWriteEnable = VK_TRUE;
  depthStencilStateCreationInformation.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencilStateCreationInformation.depthBoundsTestEnable = VK_FALSE;
  depthStencilStateCreationInformation.stencilTestEnable = VK_FALSE;
  depthStencilStateCreationInformation.minDepthBounds = 0.0f;
  depthStencilStateCreationInformation.maxDepthBounds = 1.0f;

  VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
  colorBlendAttachmentState.blendEnable = VK_FALSE;
  colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo colorBlendStateCreationInformation = {};
  colorBlendStateCreationInformation.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendStateCreationInformation.logicOpEnable = VK_FALSE;
  colorBlendStateCreationInformation.logicOp = VK_LOGIC_OP_COPY;
  colorBlendStateCreationInformation.attachmentCount = 1;
  colorBlendStateCreationInformation.pAttachments = &colorBlendAttachmentState;

  VkGraphicsPipelineCreateInfo pipelineCreationInformation = {};
  pipelineCreationInformation.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCreationInformation.pStages = shaderStagesCreationInformation;
  pipelineCreationInformation.stageCount = 2;

  pipelineCreationInformation.pVertexInputState = &vertexInputStateCreationInformation;
  pipelineCreationInformation.pInputAssemblyState = &inputAssemblyStateCreationInformation;
  pipelineCreationInformation.pViewportState = &viewportStateCreationInformation;
  pipelineCreationInformation.pRasterizationState = &rasterizationStateCreationInformation;
  pipelineCreationInformation.pMultisampleState = &multisamplingStateCreationInformation;
  pipelineCreationInformation.pColorBlendState = &colorBlendStateCreationInformation;
  pipelineCreationInformation.pDepthStencilState = &depthStencilStateCreationInformation;
  pipelineCreationInformation.pDynamicState = NULL;
  pipelineCreationInformation.layout = window->pipelineLayout;
  pipelineCreationInformation.renderPass = window->renderPass;
  pipelineCreationInformation.subpass = 0;

  if ((result = vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreationInformation, NULL, &window->pipeline)))
    return (logError("Failed to create pipeline: %s", string_VkResult(result)), destroyWindow(window), NULL);

  VkCommandPoolCreateInfo commandPoolCreationInformation = {};
  commandPoolCreationInformation.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolCreationInformation.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  commandPoolCreationInformation.queueFamilyIndex = application->graphicsQueueFamilyIndex;

  if ((result = vkCreateCommandPool(logicalDevice, &commandPoolCreationInformation, NULL, &window->commandPool)))
    return (logError("Failed to create command pool: %s", string_VkResult(result)), destroyWindow(window), NULL);

  // For now, set the maximum number of frames in flight to two
  window->maximumFramesInFlight = 2;
  if (!(window->commandBuffers = calloc(window->maximumFramesInFlight, sizeof(VkCommandBuffer)))) return (destroyWindow(window), NULL);

  VkCommandBufferAllocateInfo commandBufferAllocationInformation = {};
  commandBufferAllocationInformation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocationInformation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  commandBufferAllocationInformation.commandPool = window->commandPool;
  commandBufferAllocationInformation.commandBufferCount = window->maximumFramesInFlight;

  if ((result = vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocationInformation, window->commandBuffers)))
    return (logError("Failed to create command buffers: %s", string_VkResult(result)), destroyWindow(window), NULL);

  VkImageCreateInfo depthImageCreationInformation = {};
  depthImageCreationInformation.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  depthImageCreationInformation.imageType = VK_IMAGE_TYPE_2D;
  depthImageCreationInformation.extent.width = window->windowExtent.width;
  depthImageCreationInformation.extent.height = window->windowExtent.height;
  depthImageCreationInformation.extent.depth = 1;
  depthImageCreationInformation.mipLevels = 1;
  depthImageCreationInformation.arrayLayers = 1;
  depthImageCreationInformation.format = depthFormat;
  depthImageCreationInformation.tiling = VK_IMAGE_TILING_OPTIMAL;
  depthImageCreationInformation.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthImageCreationInformation.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  depthImageCreationInformation.samples = VK_SAMPLE_COUNT_1_BIT;
  depthImageCreationInformation.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if ((result = vkCreateImage(logicalDevice, &depthImageCreationInformation, NULL, &window->depthImage)))
    return (logError("Failed to create depth image: %s", string_VkResult(result)), destroyWindow(window), NULL);

  VkMemoryRequirements depthImageMemoryRequirements;
  vkGetImageMemoryRequirements(logicalDevice, window->depthImage, &depthImageMemoryRequirements);
  if (!memoryAllocate(window, depthImageMemoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &window->depthImageMemory))
    return (logError("Failed to allocate memory for depth image"), destroyWindow(window), NULL);

  VkImageViewCreateInfo depthImageViewCreationInformation = {};
  depthImageViewCreationInformation.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  depthImageViewCreationInformation.image = window->depthImage;
  depthImageViewCreationInformation.viewType = VK_IMAGE_VIEW_TYPE_2D;
  depthImageViewCreationInformation.format = depthFormat;
  depthImageViewCreationInformation.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  depthImageViewCreationInformation.subresourceRange.baseMipLevel = 0;
  depthImageViewCreationInformation.subresourceRange.levelCount = 1;
  depthImageViewCreationInformation.subresourceRange.baseArrayLayer = 0;
  depthImageViewCreationInformation.subresourceRange.layerCount = 1;

  if ((result = vkCreateImageView(logicalDevice, &depthImageViewCreationInformation, NULL, &window->depthImageView)))
    return (logError("Failed to create depth image view: %s", string_VkResult(result)), destroyWindow(window), NULL);

  const char *const imageFilePath = "resources/models/pirate_ship/Textures/colormap.png";
  if (!textureLoad(window, imageFilePath, &window->textureImage, &window->textureImageMemory))
    return (logError("Failed to load texture image"), destroyWindow(window), NULL);

  VkImageViewCreateInfo imageViewCreationInformation = {};
  imageViewCreationInformation.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewCreationInformation.image = window->textureImage;
  imageViewCreationInformation.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewCreationInformation.format = VK_FORMAT_R8G8B8A8_SRGB;
  imageViewCreationInformation.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageViewCreationInformation.subresourceRange.baseMipLevel = 0;
  imageViewCreationInformation.subresourceRange.levelCount = 1;
  imageViewCreationInformation.subresourceRange.baseArrayLayer = 0;
  imageViewCreationInformation.subresourceRange.layerCount = 1;

  if ((result = vkCreateImageView(logicalDevice, &imageViewCreationInformation, NULL, &window->textureImageView)))
    return (logError("Failed to create texture image view: %s", string_VkResult(result)), destroyWindow(window), NULL);

  VkSamplerCreateInfo samplerCreationInformation = {};
  samplerCreationInformation.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCreationInformation.magFilter = VK_FILTER_LINEAR;
  samplerCreationInformation.minFilter = VK_FILTER_LINEAR;
  samplerCreationInformation.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreationInformation.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreationInformation.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreationInformation.anisotropyEnable = VK_FALSE;
  samplerCreationInformation.maxAnisotropy = 1.0f;
  samplerCreationInformation.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerCreationInformation.unnormalizedCoordinates = VK_FALSE;
  samplerCreationInformation.compareEnable = VK_FALSE;
  samplerCreationInformation.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerCreationInformation.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCreationInformation.mipLodBias = 0.0f;
  samplerCreationInformation.minLod = 0.0f;
  samplerCreationInformation.maxLod = 0.0f;

  if ((result = vkCreateSampler(logicalDevice, &samplerCreationInformation, NULL, &window->textureSampler)))
    return (logError("Failed to create texture sampler: %s", string_VkResult(result)), destroyWindow(window), NULL);

  struct Object *const object = createObject("resources/models/pirate_ship/ship-pirate-large.obj");
  if (!object) return (logError("Failed to create object"), destroyWindow(window), NULL);

  // For now, this is the same as the index count because I will implement vertex deduplication later
  window->indexCount = object->indices->length / 3;
  const VkDeviceSize vertexBufferSize = window->indexCount * VERTEX_SIZE;
  const VkDeviceSize indexBufferSize = window->indexCount * INDEX_SIZE;

  VkBuffer vertexStagingBuffer;
  VkDeviceMemory vertexStagingBufferMemory;
  const VkBufferUsageFlagBits vertexStagingBufferUsages = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  const VkMemoryPropertyFlags vertexStagingBufferMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  if (!bufferCreate(window, vertexBufferSize, vertexStagingBufferUsages, vertexStagingBufferMemoryProperties, &vertexStagingBuffer, &vertexStagingBufferMemory))
    return (logError("Failed to create vertex staging buffer"), destroyWindow(window), destroyObject(object), NULL);

  void *vertexStagingBufferData;
  if ((result = vkMapMemory(logicalDevice, vertexStagingBufferMemory, 0, vertexBufferSize, 0, &vertexStagingBufferData))) {
    logError("Failed to map staging buffer memory: %s", string_VkResult(result));
    vkDestroyBuffer(logicalDevice, vertexStagingBuffer, NULL);
    vkFreeMemory(logicalDevice, vertexStagingBufferMemory, NULL);
    destroyWindow(window);
    destroyObject(object);
    return NULL;
  }

  VkBuffer indexStagingBuffer;
  VkDeviceMemory indexStagingBufferMemory;
  const VkBufferUsageFlagBits indexStagingBufferUsages = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  const VkMemoryPropertyFlags indexStagingBufferMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  if (!bufferCreate(window, indexBufferSize, indexStagingBufferUsages, indexStagingBufferMemoryProperties, &indexStagingBuffer, &indexStagingBufferMemory)) {
    logError("Failed to create index staging buffer");
    vkDestroyBuffer(logicalDevice, vertexStagingBuffer, NULL);
    vkFreeMemory(logicalDevice, vertexStagingBufferMemory, NULL);
    destroyWindow(window);
    destroyObject(object);
    return NULL;
  }

  void *indexStagingBufferData;
  if ((result = vkMapMemory(logicalDevice, indexStagingBufferMemory, 0, indexBufferSize, 0, &indexStagingBufferData))) {
    logError("Failed to map staging buffer memory: %s", string_VkResult(result));
    vkDestroyBuffer(logicalDevice, vertexStagingBuffer, NULL);
    vkFreeMemory(logicalDevice, vertexStagingBufferMemory, NULL);
    vkDestroyBuffer(logicalDevice, indexStagingBuffer, NULL);
    vkFreeMemory(logicalDevice, indexStagingBufferMemory, NULL);
    destroyWindow(window);
    destroyObject(object);
    return NULL;
  }

  const float *positions = (float *)object->positions->data;
  const float *normals = (float *)object->normals->data;
  const float *textureCoordinates = (float *)object->textureCoordinates->data;
  const uint32_t *indices = (uint32_t *)object->indices->data;

  for (size_t i = 0; i < object->indices->length / 9; ++i) {
    for (size_t j = 0; j < 3; ++j) {
      const size_t parsedIndexOffset = 9 * i + 3 * j;
      const uint32_t positionIndex           = 3 * indices[parsedIndexOffset + 0];
      const uint32_t textureCoordinatesIndex = 3 * indices[parsedIndexOffset + 1];
      const uint32_t normalIndex             = 3 * indices[parsedIndexOffset + 2];

      const float vertex[13] = {
        positions[positionIndex + 0],
        positions[positionIndex + 1],
        positions[positionIndex + 2],
        normals[normalIndex + 0],
        normals[normalIndex + 1],
        normals[normalIndex + 2],
        0.0f + textureCoordinates[textureCoordinatesIndex + 0],
        1.0f - textureCoordinates[textureCoordinatesIndex + 1],
        0.0f + textureCoordinates[textureCoordinatesIndex + 2],
        0.0f, 0.0f, 0.0f, 0.0f
      };

      memcpy((float *)vertexStagingBufferData + 13 * (3 * i + j), &vertex, sizeof(vertex));
      ((uint32_t *)indexStagingBufferData)[3 * i + j] = (uint32_t)(3 * i + j);
    }
  }

  destroyObject(object);
  vkUnmapMemory(logicalDevice, vertexStagingBufferMemory);
  vkUnmapMemory(logicalDevice, indexStagingBufferMemory);
  
  const VkBufferUsageFlagBits vertexBufferUsages = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  const VkMemoryPropertyFlags vertexBufferMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  if (!bufferCreate(window, vertexBufferSize, vertexBufferUsages, vertexBufferMemoryProperties, &window->vertexBuffer, &window->vertexBufferMemory)) {
    logError("Failed to create vertex buffer");
    vkDestroyBuffer(logicalDevice, vertexStagingBuffer, NULL);
    vkFreeMemory(logicalDevice, vertexStagingBufferMemory, NULL);
    vkDestroyBuffer(logicalDevice, indexStagingBuffer, NULL);
    vkFreeMemory(logicalDevice, indexStagingBufferMemory, NULL);
    destroyWindow(window);
    return NULL;
  }

  if (!bufferCopy(window, vertexStagingBuffer, window->vertexBuffer, vertexBufferSize)) {
    logError("Failed to copy staging buffer onto vertex buffer");
    vkDestroyBuffer(logicalDevice, vertexStagingBuffer, NULL);
    vkFreeMemory(logicalDevice, vertexStagingBufferMemory, NULL);
    vkDestroyBuffer(logicalDevice, indexStagingBuffer, NULL);
    vkFreeMemory(logicalDevice, indexStagingBufferMemory, NULL);
    destroyWindow(window);
    return NULL;
  }

  vkDestroyBuffer(logicalDevice, vertexStagingBuffer, NULL);
  vkFreeMemory(logicalDevice, vertexStagingBufferMemory, NULL);
  
  const VkBufferUsageFlagBits indexBufferUsages = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  const VkMemoryPropertyFlags indexBufferMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  if (!bufferCreate(window, indexBufferSize, indexBufferUsages, indexBufferMemoryProperties, &window->indexBuffer, &window->indexBufferMemory)) {
    logError("Failed to create index buffer");
    vkDestroyBuffer(logicalDevice, indexStagingBuffer, NULL);
    vkFreeMemory(logicalDevice, indexStagingBufferMemory, NULL);
    destroyWindow(window);
    destroyObject(object);
    return NULL;
  }

  if (!bufferCopy(window, indexStagingBuffer, window->indexBuffer, indexBufferSize)) {
    logError("Failed to copy staging buffer onto index buffer");
    vkDestroyBuffer(logicalDevice, indexStagingBuffer, NULL);
    vkFreeMemory(logicalDevice, indexStagingBufferMemory, NULL);
    destroyWindow(window);
    destroyObject(object);
    return NULL;
  }

  vkDestroyBuffer(logicalDevice, indexStagingBuffer, NULL);
  vkFreeMemory(logicalDevice, indexStagingBufferMemory, NULL);

  const size_t uniformBufferCount = (size_t)window->maximumFramesInFlight;
  const VkDeviceSize uniformBufferSize = sizeof(window->uniformBufferObject);

  if (!(window->uniformBuffers = calloc(uniformBufferCount, sizeof(VkBuffer)))) return (destroyWindow(window), NULL);
  if (!(window->uniformBuffersMemory = calloc(uniformBufferCount, sizeof(VkDeviceMemory)))) return (destroyWindow(window), NULL);
  if (!(window->uniformBuffersMapped = calloc(uniformBufferCount, sizeof(void *)))) return (destroyWindow(window), NULL);

  for (size_t i = 0; i < uniformBufferCount; ++i) {
    const VkBufferUsageFlagBits uniformBufferUsages = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    const VkMemoryPropertyFlags uniformBufferMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    if (!bufferCreate(window, uniformBufferSize, uniformBufferUsages, uniformBufferMemoryProperties, &window->uniformBuffers[i], &window->uniformBuffersMemory[i]))
      return (logError("Failed to create uniform buffer #%d", i), destroyWindow(window), NULL);
    if ((result = vkMapMemory(logicalDevice, window->uniformBuffersMemory[i], 0, uniformBufferSize, 0, &window->uniformBuffersMapped[i])))
      return (logError("Failed to map memory for uniform buffer #%d: %s", i, string_VkResult(result)), destroyWindow(window), NULL);
  }

  VkDescriptorPoolSize descriptorPoolSizes[2];
  descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorPoolSizes[0].descriptorCount = (uint32_t)window->maximumFramesInFlight;
  descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorPoolSizes[1].descriptorCount = (uint32_t)window->maximumFramesInFlight;

  VkDescriptorPoolCreateInfo descriptorPoolCreationInformation = {};
  descriptorPoolCreationInformation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCreationInformation.maxSets = (uint16_t)window->maximumFramesInFlight;
  descriptorPoolCreationInformation.pPoolSizes = descriptorPoolSizes;
  descriptorPoolCreationInformation.poolSizeCount = 2;

  if ((result = vkCreateDescriptorPool(logicalDevice, &descriptorPoolCreationInformation, NULL, &window->descriptorPool)))
    return (logError("Failed to create descrioptor pool: %s", string_VkResult(result)), destroyWindow(window), NULL);

  VkDescriptorSetAllocateInfo descriptorSetAllocationInformation = {};
  descriptorSetAllocationInformation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorSetAllocationInformation.descriptorPool = window->descriptorPool;
  descriptorSetAllocationInformation.descriptorSetCount = (uint32_t)window->maximumFramesInFlight;

  // TODO: Address this issue: I am not sure why, but it thinks that I have multiple (the number of
  // maximum frames in flight, to be precise) descriptor set layouts, and I can't get to fix this.
  // So this array is used temporarly to fill in the array of descriptor set layouts expected in
  // `descriptorSetAllocationInformation.pSetLayouts` 
  VkDescriptorSetLayout descriptorSetLayouts[window->maximumFramesInFlight];

  for (uint32_t i = 0; i < window->maximumFramesInFlight; ++i) descriptorSetLayouts[i] = window->descriptorSetLayout;
  descriptorSetAllocationInformation.pSetLayouts = descriptorSetLayouts;

  if (!(window->descriptorSets = calloc(window->maximumFramesInFlight, sizeof(VkDescriptorSet)))) return (destroyWindow(window), NULL);
  if ((result = vkAllocateDescriptorSets(logicalDevice, &descriptorSetAllocationInformation, window->descriptorSets)))
    return (logError("Failed to allocate descriptor sets: %s", string_VkResult(result)) ,destroyWindow(window), NULL);

  for (uint32_t i = 0; i < window->maximumFramesInFlight; ++i) {
    VkDescriptorBufferInfo descriptorBufferInformation = {};
    descriptorBufferInformation.range = sizeof(window->uniformBufferObject);
    descriptorBufferInformation.buffer = window->uniformBuffers[i];
    descriptorBufferInformation.offset = 0;

    VkDescriptorImageInfo descriptorImageInformation = {};
    descriptorImageInformation.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorImageInformation.imageView = window->textureImageView;
    descriptorImageInformation.sampler = window->textureSampler;

    VkDescriptorSet descriptorSet = window->descriptorSets[i];
    VkWriteDescriptorSet writeDescriptorSets[2];

    writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[0].dstSet = descriptorSet;
    writeDescriptorSets[0].dstBinding = 0;
    writeDescriptorSets[0].dstArrayElement = 0;
    writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSets[0].descriptorCount = 1;
    writeDescriptorSets[0].pBufferInfo = &descriptorBufferInformation;
    writeDescriptorSets[0].pNext = NULL;

    writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[1].dstSet = descriptorSet;
    writeDescriptorSets[1].dstBinding = 1;
    writeDescriptorSets[1].dstArrayElement = 0;
    writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSets[1].descriptorCount = 1;
    writeDescriptorSets[1].pImageInfo = &descriptorImageInformation;
    writeDescriptorSets[1].pNext = NULL;

    vkUpdateDescriptorSets(logicalDevice, 2, writeDescriptorSets, 0, NULL);
  }

  if (!(window->framebuffers = calloc(window->imageCount, sizeof(VkFramebuffer)))) return (destroyWindow(window), NULL);
  for (uint32_t i = 0; i < window->imageCount; ++i) {
    const VkImageView framebufferAttachments[] = {window->imageViews[i], window->depthImageView};
    const size_t framebufferAttachmentCount = sizeof(framebufferAttachments) / sizeof(framebufferAttachments[0]);

    VkFramebufferCreateInfo framebufferCreationInformation = {};
    framebufferCreationInformation.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreationInformation.renderPass = window->renderPass;
    framebufferCreationInformation.attachmentCount = framebufferAttachmentCount;
    framebufferCreationInformation.pAttachments = framebufferAttachments;
    framebufferCreationInformation.width = width;
    framebufferCreationInformation.height = height;
    framebufferCreationInformation.layers = 1;

    if ((result = vkCreateFramebuffer(logicalDevice, &framebufferCreationInformation, NULL, &window->framebuffers[i])))
      return (logError("Failed to create framebuffer %d: %s", i, string_VkResult(result)), destroyWindow(window), NULL);
  }

  if (!(window->imageAvailableSemaphores = calloc(window->maximumFramesInFlight, sizeof(VkSemaphore)))) return (destroyWindow(window), NULL);
  if (!(window->renderFinishedSemaphores = calloc(window->maximumFramesInFlight, sizeof(VkSemaphore)))) return (destroyWindow(window), NULL);
  if (!(window->inFlightFences = calloc(window->maximumFramesInFlight, sizeof(VkFence)))) return (destroyWindow(window), NULL);

  VkSemaphoreCreateInfo semaphoreCreationInformation = {};
  semaphoreCreationInformation.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceCreationInformation = {};
  fenceCreationInformation.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreationInformation.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (uint32_t i = 0; i < window->maximumFramesInFlight; ++i) {
    if ((result = vkCreateSemaphore(logicalDevice, &semaphoreCreationInformation, NULL, &window->imageAvailableSemaphores[i])))
      return (logError("Failed to create image available semaphore #%d: %s", i, string_VkResult(result)), destroyWindow(window), NULL);
    if ((result = vkCreateSemaphore(logicalDevice, &semaphoreCreationInformation, NULL, &window->renderFinishedSemaphores[i])))
      return (logError("Failed to create render finished semaphore #%d: %s", i, string_VkResult(result)), destroyWindow(window), NULL);
    if ((result = vkCreateFence(logicalDevice, &fenceCreationInformation, NULL, &window->inFlightFences[i])))
      return (logError("Failed to create in flight fence #%d: %s", i, string_VkResult(result)), destroyWindow(window), NULL);
  }

  return window;
}

bool updateWindow(struct Window *const window) {
  if (window->pendingFrame && !windowEndFrame(window)) return false;
  window->pendingFrame = true;

  // Move to the next frame to get the corresponding command buffer, and synchronization objects
  window->frameIndex = (window->frameIndex + 1) % window->maximumFramesInFlight;
  return windowBeginFrame(window);
}

static inline bool windowBeginFrame(struct Window *const window) {
  VkResult result;

  VkDevice const logicalDevice = window->application->logicalDevice;
  VkCommandBuffer const commandBuffer = window->commandBuffers[window->frameIndex];
  VkSemaphore const imageAvailableSemaphore = window->imageAvailableSemaphores[window->frameIndex];
  VkSemaphore const renderFinishedSemaphore = window->renderFinishedSemaphores[window->frameIndex];
  VkFence const inFlightFence = window->inFlightFences[window->frameIndex];

  if ((result = vkWaitForFences(logicalDevice, 1, &inFlightFence, VK_TRUE, UINT64_MAX)))
    return (logError("Failed to wait for in flight fence #%d: %s", window->frameIndex, string_VkResult(result)), false);

  if ((result = vkResetFences(logicalDevice, 1, &inFlightFence)))
    return (logError("Failed to reset in flight fence #%d: %s", window->frameIndex, string_VkResult(result)), false);

  if ((result = vkAcquireNextImageKHR(logicalDevice, window->swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &window->imageIndex)))
    return (logError("Failed to acquire next swapchain image: %s", string_VkResult(result)), false);

  if ((result = vkResetCommandBuffer(commandBuffer, 0)))
    return (logError("Failed to reset command buffer #%d: %s", window->frameIndex, string_VkResult(result)), false);

  VkCommandBufferBeginInfo commandBufferBeginningInformation = {};
  commandBufferBeginningInformation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if ((result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginningInformation)))
    return (logError("Failed to begin command buffer #%d: %s", window->frameIndex, string_VkResult(result)), 0);

  return 1;
}

static inline bool windowEndFrame(const struct Window *const window) {
  VkResult result;

  struct Application const *application = window->application;
  VkDevice const logicalDevice = application->logicalDevice;
  VkCommandBuffer const commandBuffer = window->commandBuffers[window->frameIndex];
  VkSemaphore const imageAvailableSemaphore = window->imageAvailableSemaphores[window->frameIndex];
  VkSemaphore const renderFinishedSemaphore = window->renderFinishedSemaphores[window->frameIndex];
  VkFence const inFlightFence = window->inFlightFences[window->frameIndex];

  if ((result = vkEndCommandBuffer(commandBuffer)))
    return (logError("Failed to end command buffer #%d: %s", window->frameIndex, string_VkResult(result)), false);

  VkSubmitInfo submissionInformation = {};
  submissionInformation.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submissionInformation.pCommandBuffers = &commandBuffer;
  submissionInformation.commandBufferCount = 1;
  submissionInformation.pWaitSemaphores = &imageAvailableSemaphore;
  submissionInformation.waitSemaphoreCount = 1;
  submissionInformation.pSignalSemaphores = &renderFinishedSemaphore;
  submissionInformation.signalSemaphoreCount = 1;

  const VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submissionInformation.pWaitDstStageMask = waitStages;

  if ((result = vkQueueSubmit(application->graphicsQueue, 1, &submissionInformation, inFlightFence)))
    return (logError("Failed to submit command buffer #%d: %s", window->frameIndex, string_VkResult(result)), false);

  VkPresentInfoKHR presentationInformation = {};
  presentationInformation.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentationInformation.pSwapchains = &window->swapchain;
  presentationInformation.swapchainCount = 1;
  presentationInformation.pImageIndices = &window->imageIndex;
  presentationInformation.pWaitSemaphores = &renderFinishedSemaphore;
  presentationInformation.waitSemaphoreCount = 1;
  presentationInformation.pResults = NULL;

  if ((result = vkQueuePresentKHR(application->presentQueue, &presentationInformation)))
    return (logError("Failed to present image: %s", window->frameIndex, string_VkResult(result)), false);

  return true;
}

void drawSomething(struct Window *const window) {
  VkCommandBuffer const commandBuffer = window->commandBuffers[window->frameIndex];

  VkRenderPassBeginInfo renderPassBeginningInformation = {};
  renderPassBeginningInformation.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginningInformation.renderPass = window->renderPass;
  renderPassBeginningInformation.framebuffer = window->framebuffers[window->frameIndex];
  renderPassBeginningInformation.renderArea.extent = window->windowExtent;
  renderPassBeginningInformation.renderArea.offset.x = 0;
  renderPassBeginningInformation.renderArea.offset.y = 0;

  VkClearValue clearValues[2];
  clearValues[0].color.float32[0] = 0.0f;
  clearValues[0].color.float32[1] = 0.0f;
  clearValues[0].color.float32[2] = 0.0f;
  clearValues[0].color.float32[3] = 1.0f;
  clearValues[1].depthStencil.depth = 1.0f;
  clearValues[1].depthStencil.stencil = 0;

  renderPassBeginningInformation.pClearValues = clearValues;
  renderPassBeginningInformation.clearValueCount = 2;

  vkCmdBeginRenderPass(commandBuffer, &renderPassBeginningInformation, VK_SUBPASS_CONTENTS_INLINE);

  const float radians = acos(-1.0) / 180.0f;

  struct timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);
  const float currentTime = time.tv_sec + time.tv_nsec / 1e9;

  const float rotationAxis[3] = {0.0f, 1.0f, 0.0f};
  matrixRotate4x4(window->uniformBufferObject.model, currentTime * radians * 90.0f, rotationAxis);

  const float pov[3] = {20.0f, 20.0f, 10.0f};
  const float center[3] = {0.0f, 0.0f, 0.0f};
  const float up[3] = {0.0f, 1.0f, 0.0f};
  matrixView4x4(window->uniformBufferObject.view, pov, center, up);

  float aspectRatio = window->windowExtent.width / (float)window->windowExtent.height;
  matrixPerspective4x4(window->uniformBufferObject.projection, radians * 45.0f, aspectRatio, 0.05f, 1000.0f);

  window->uniformBufferObject.projection[1][1] *= -1.0f;

  void *uniformBufferMemory = window->uniformBuffersMapped[window->frameIndex];
  memcpy(uniformBufferMemory, &window->uniformBufferObject, sizeof(window->uniformBufferObject));

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, window->pipeline);

  VkDeviceSize offsets[1] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &window->vertexBuffer, offsets);

  vkCmdBindIndexBuffer(commandBuffer, window->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, window->pipelineLayout, 0, 1, &window->descriptorSets[window->frameIndex], 0, NULL);

  vkCmdDrawIndexed(commandBuffer, window->indexCount, 1, 0, 0, 0);

  vkCmdEndRenderPass(commandBuffer);
}

void destroyWindow(struct Window *const window) {
  if (!window) return;

  VkResult result;

  struct Application const *application = window->application;
  VkInstance const vulkanInstance = application->vulkanInstance;
  VkDevice const logicalDevice = application->logicalDevice;
  VkCommandBuffer *commandBuffers = window->commandBuffers;

  if (window->pendingFrame && (result = vkEndCommandBuffer(commandBuffers[window->frameIndex])))
    logWarning("Failed to end last command buffer #%d: %s", window->frameIndex, string_VkResult(result));

  vkDeviceWaitIdle(logicalDevice);
  for (uint32_t i = 0; i < window->maximumFramesInFlight; ++i) {
    vkDestroyFence(logicalDevice, window->inFlightFences[i], NULL);
    vkDestroySemaphore(logicalDevice, window->renderFinishedSemaphores[i], NULL);
    vkDestroySemaphore(logicalDevice, window->imageAvailableSemaphores[i], NULL);
    vkDestroyFramebuffer(logicalDevice, window->framebuffers[i], NULL);
    vkDestroyBuffer(logicalDevice, window->uniformBuffers[i], NULL);
    vkFreeMemory(logicalDevice, window->uniformBuffersMemory[i], NULL);
    free(window->uniformBuffersMapped[i]);
  }

  free(window->inFlightFences);
  free(window->renderFinishedSemaphores);
  free(window->imageAvailableSemaphores);
  free(window->framebuffers);
  free(window->uniformBuffersMapped);
  free(window->uniformBuffersMemory);
  free(window->uniformBuffers);
  free(window->descriptorSets);
  vkDestroyDescriptorPool(logicalDevice, window->descriptorPool, NULL);
  vkDestroyDescriptorSetLayout(logicalDevice, window->descriptorSetLayout, NULL);
  vkDestroyBuffer(logicalDevice, window->indexBuffer, NULL);
  vkFreeMemory(logicalDevice, window->indexBufferMemory, NULL);
  vkDestroyBuffer(logicalDevice, window->vertexBuffer, NULL);
  vkFreeMemory(logicalDevice, window->vertexBufferMemory, NULL);
  vkDestroySampler(logicalDevice, window->textureSampler, NULL);
  vkDestroyImageView(logicalDevice, window->textureImageView, NULL);
  vkDestroyImage(logicalDevice, window->textureImage, NULL);
  vkFreeMemory(logicalDevice, window->textureImageMemory, NULL);
  vkDestroyImageView(logicalDevice, window->depthImageView, NULL);
  vkDestroyImage(logicalDevice, window->depthImage, NULL);
  vkFreeMemory(logicalDevice, window->depthImageMemory, NULL);
  vkDestroyCommandPool(logicalDevice, window->commandPool, NULL);
  free(commandBuffers);
  vkDestroyPipeline(logicalDevice, window->pipeline, NULL);
  vkDestroyPipelineLayout(logicalDevice, window->pipelineLayout, NULL);
  for (size_t i = 0; i < window->shaderModuleCount; ++i) vkDestroyShaderModule(logicalDevice, window->shaderModules[i], NULL);
  free(window->shaderModules);
  vkDestroyRenderPass(logicalDevice, window->renderPass, NULL);
  for (uint32_t i = 0; i < window->imageCount; ++i) vkDestroyImageView(logicalDevice, window->imageViews[i], NULL);
  free(window->imageViews);
  free(window->images);
  vkDestroySwapchainKHR(logicalDevice, window->swapchain, NULL);
  SDL_Vulkan_DestroySurface(vulkanInstance, window->surface, NULL);
  SDL_DestroyWindow(window->sdlWindowHandle);
  free(window);
}

static inline bool memoryAllocate(const struct Window *const window, const VkMemoryRequirements requirements, const VkMemoryPropertyFlags properties, VkDeviceMemory *memory) {
  VkResult result;

  VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
  vkGetPhysicalDeviceMemoryProperties(window->application->physicalDevice, &physicalDeviceMemoryProperties);

  VkMemoryAllocateInfo memoryAllocationInformation = {};
  memoryAllocationInformation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memoryAllocationInformation.allocationSize = requirements.size;

  memoryAllocationInformation.memoryTypeIndex = UINT32_MAX;
  for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i) {
    if ((requirements.memoryTypeBits & (1 << i)) && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      memoryAllocationInformation.memoryTypeIndex = i;
      break;
    }
  }

  if (memoryAllocationInformation.memoryTypeIndex == UINT32_MAX)
    return (logError("Failed to find suitable physical device memory type for memory allocation"), false);

  if ((result = vkAllocateMemory(window->application->logicalDevice, &memoryAllocationInformation, NULL, memory)))
    return (logError("Failed to allocate memory: %s", string_VkResult(result)), false);

  return true;
}

static inline bool commandStart(const struct Window *const window, VkCommandBuffer *commandBuffer) {
  VkResult result;

  VkCommandBufferAllocateInfo commandBufferAllocationInformation = {};
  commandBufferAllocationInformation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocationInformation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  commandBufferAllocationInformation.commandPool = window->commandPool;
  commandBufferAllocationInformation.commandBufferCount = 1;

  if ((result = vkAllocateCommandBuffers(window->application->logicalDevice, &commandBufferAllocationInformation, commandBuffer)))
    return (logError("Failed to allocate single time command buffer: %s", string_VkResult(result)), false);

  VkCommandBufferBeginInfo commandBufferBeginningInformation = {};
  commandBufferBeginningInformation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  commandBufferBeginningInformation.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if ((result = vkBeginCommandBuffer(*commandBuffer, &commandBufferBeginningInformation))) {
    logError("Failed to begin single time command buffer: %s", string_VkResult(result));
    vkFreeCommandBuffers(window->application->logicalDevice, window->commandPool, 1, commandBuffer);
    return false;
  }

  return true;
}

static inline bool commandFinish(const struct Window *const window, VkCommandBuffer commandBuffer) {
  VkResult result;

  if ((result = vkEndCommandBuffer(commandBuffer))) {
    logError("Failed to end single time command buffer: %s", string_VkResult(result));
    vkFreeCommandBuffers(window->application->logicalDevice, window->commandPool, 1, &commandBuffer);
    return false;
  }

  VkSubmitInfo submissionInformation = {};
  submissionInformation.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submissionInformation.commandBufferCount = 1;
  submissionInformation.pCommandBuffers = &commandBuffer;

  if ((result = vkQueueSubmit(window->application->graphicsQueue, 1, &submissionInformation, VK_NULL_HANDLE))) {
    logError("Failed to submit single time command buffer to graphics queue: %s", string_VkResult(result));
    vkFreeCommandBuffers(window->application->logicalDevice, window->commandPool, 1, &commandBuffer);
    return false;
  }

  if ((result = vkQueueWaitIdle(window->application->graphicsQueue))) {
    logError("Failed to wait for graphics queue: %s", string_VkResult(result));
    vkFreeCommandBuffers(window->application->logicalDevice, window->commandPool, 1, &commandBuffer);
    return false;
  }

  vkFreeCommandBuffers(window->application->logicalDevice, window->commandPool, 1, &commandBuffer);
  return true;
}

static inline bool bufferCreate(const struct Window *const window, const VkDeviceSize size, const VkBufferUsageFlags usages, const VkMemoryPropertyFlags memoryProperties, VkBuffer *buffer, VkDeviceMemory *memory) {
  VkResult result;

  VkBufferCreateInfo bufferCreationInformation = {};
  bufferCreationInformation.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreationInformation.size = size;
  bufferCreationInformation.usage = usages;
  bufferCreationInformation.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if ((result = vkCreateBuffer(window->application->logicalDevice, &bufferCreationInformation, NULL, buffer)))
    return (logError("Failed to create buffer: %s", string_VkResult(result)), false);

  VkMemoryRequirements memoryRequirements;
  vkGetBufferMemoryRequirements(window->application->logicalDevice, *buffer, &memoryRequirements);
  if (!memoryAllocate(window, memoryRequirements, memoryProperties, memory)) {
    logError("Failed to allocate memory for buffer: %s", string_VkResult(result));
    vkDestroyBuffer(window->application->logicalDevice, *buffer, NULL);
    return false;
  }

  if ((result = vkBindBufferMemory(window->application->logicalDevice, *buffer, *memory, 0))) {
    logError("Failed to bind buffer memory: %s", string_VkResult(result));
    vkDestroyBuffer(window->application->logicalDevice, *buffer, NULL);
    vkFreeMemory(window->application->logicalDevice, *memory, NULL);
    return false;
  }

  return true;
}

static inline bool bufferCopy(const struct Window *const window, VkBuffer source, VkBuffer destination, const VkDeviceSize size) {
  VkCommandBuffer commandBuffer;
  if (!commandStart(window, &commandBuffer)) return (logError("Failed to start single time command buffer"), false);

  VkBufferCopy bufferCopyRegion = {};
  bufferCopyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, source, destination, 1, &bufferCopyRegion);

  if (!commandFinish(window, commandBuffer)) return (logError("Failed to finish single time command buffer"), false);
  return true;
}

static inline bool imageLayoutTransition(const struct Window *const window, VkImage image, const VkFormat format, const VkImageLayout previousLayout, const VkImageLayout nextLayout);
static inline bool bufferCopyToImage(const struct Window *const window, VkBuffer buffer, VkImage image, const uint32_t width, const uint32_t height);
static inline bool textureLoad(const struct Window *const window, const char *const path, VkImage *texture, VkDeviceMemory *memory) {
  VkResult result;

  struct Image *const image = createImage(path);
  if (!image) return (logError("Failed to create image"), false);

  const VkDeviceSize imageSize = 4 * image->width * image->height;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  const VkBufferUsageFlagBits stagingBufferUsages = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  const VkMemoryPropertyFlags stagingBufferMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  if (!bufferCreate(window, imageSize, stagingBufferUsages, stagingBufferMemoryProperties, &stagingBuffer, &stagingBufferMemory))
    return (logError("Failed to create staging buffer for image"), destroyImage(image), 0);

  void *stagingBufferData;
  if ((result = vkMapMemory(window->application->logicalDevice, stagingBufferMemory, 0, imageSize, 0, &stagingBufferData))) {
    logError("Failed to map staging buffer memory: %s", string_VkResult(result));
    vkDestroyBuffer(window->application->logicalDevice, stagingBuffer, NULL);
    destroyImage(image);
    return 0;
  }

  for (size_t i = 0; i < image->height; ++i) memcpy((uint8_t *)stagingBufferData + i * image->width * 4, image->data[i], image->width * 4);
  vkUnmapMemory(window->application->logicalDevice, stagingBufferMemory);

  VkImageCreateInfo imageCreationInformation = {};
  imageCreationInformation.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreationInformation.imageType = VK_IMAGE_TYPE_2D;
  imageCreationInformation.extent.width = (uint32_t)image->width;
  imageCreationInformation.extent.height = (uint32_t)image->height;
  imageCreationInformation.extent.depth = 1;
  imageCreationInformation.mipLevels = 1;
  imageCreationInformation.arrayLayers = 1;
  imageCreationInformation.format = VK_FORMAT_R8G8B8A8_SRGB;
  imageCreationInformation.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreationInformation.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreationInformation.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageCreationInformation.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreationInformation.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreationInformation.flags = 0;

  if ((result = vkCreateImage(window->application->logicalDevice, &imageCreationInformation, NULL, texture))) {
    logError("Failed to create texture: %s", string_VkResult(result));
    vkDestroyBuffer(window->application->logicalDevice, stagingBuffer, NULL);
    vkFreeMemory(window->application->logicalDevice, stagingBufferMemory, NULL);
    destroyImage(image);
    return false;
  }

  VkMemoryRequirements imageMemoryRequirements;
  vkGetImageMemoryRequirements(window->application->logicalDevice, *texture, &imageMemoryRequirements);

  if (!memoryAllocate(window, imageMemoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory)) {
    logError("Failed to allocate memory for image");
    vkDestroyBuffer(window->application->logicalDevice, stagingBuffer, NULL);
    vkFreeMemory(window->application->logicalDevice, stagingBufferMemory, NULL);
    destroyImage(image);
    return false;
  }

  if ((result = vkBindImageMemory(window->application->logicalDevice, *texture, *memory, 0))) {
    logError("Failed to bind image memory: %s", string_VkResult(result));
    vkDestroyBuffer(window->application->logicalDevice, stagingBuffer, NULL);
    vkFreeMemory(window->application->logicalDevice, stagingBufferMemory, NULL);
    destroyImage(image);
    return false;
  }

  if (!imageLayoutTransition(window, *texture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)) {
    logError("Failed to transition image layout");
    vkDestroyBuffer(window->application->logicalDevice, stagingBuffer, NULL);
    vkFreeMemory(window->application->logicalDevice, stagingBufferMemory, NULL);
    destroyImage(image);
    return false;
  }

  if (!bufferCopyToImage(window, stagingBuffer, *texture, (uint32_t)image->width, (uint32_t)image->height)) {
    logError("Failed to copy buffer to image");
    vkDestroyBuffer(window->application->logicalDevice, stagingBuffer, NULL);
    vkFreeMemory(window->application->logicalDevice, stagingBufferMemory, NULL);
    destroyImage(image);
    return false;
  }

  if (!imageLayoutTransition(window, *texture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
    logError("Failed to transition image layout");
    vkDestroyBuffer(window->application->logicalDevice, stagingBuffer, NULL);
    vkFreeMemory(window->application->logicalDevice, stagingBufferMemory, NULL);
    destroyImage(image);
    return false;
  }

  vkDestroyBuffer(window->application->logicalDevice, stagingBuffer, NULL);
  vkFreeMemory(window->application->logicalDevice, stagingBufferMemory, NULL);
  destroyImage(image);
  return true;
}

static inline bool imageLayoutTransition(const struct Window *const window, VkImage image, const VkFormat format, const VkImageLayout previousLayout, const VkImageLayout nextLayout) {
  VkCommandBuffer commandBuffer;
  if (!commandStart(window, &commandBuffer)) return (logError("Failed to start single time command buffer"), false);

  VkImageMemoryBarrier imageMemoryBarrier = {};
  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemoryBarrier.oldLayout = previousLayout;
  imageMemoryBarrier.newLayout = nextLayout;
  imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.image = image;
  imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
  imageMemoryBarrier.subresourceRange.levelCount = 1;
  imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
  imageMemoryBarrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStages;
  VkPipelineStageFlags destinationStages;

  if (previousLayout == VK_IMAGE_LAYOUT_UNDEFINED && nextLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceStages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStages = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (previousLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && nextLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStages = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else return (logError("Unsupported image layout transition from '%s' to '%s'", string_VkImageLayout(previousLayout), string_VkImageLayout(nextLayout)), commandFinish(window, commandBuffer), false);

  vkCmdPipelineBarrier(commandBuffer, sourceStages, destinationStages, 0, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);
  if (!commandFinish(window, commandBuffer)) return (logError("Failed to finish single time command buffer"), false);
  return true;
}

static inline bool bufferCopyToImage(const struct Window *const window, VkBuffer buffer, VkImage image, const uint32_t width, const uint32_t height) {
  VkCommandBuffer commandBuffer;
  if (!commandStart(window, &commandBuffer)) return (logError("Failed to start single time command buffer"), false);

  VkBufferImageCopy bufferImageCopyRegion = {};
  bufferImageCopyRegion.bufferOffset = 0;
  bufferImageCopyRegion.bufferRowLength = 0;
  bufferImageCopyRegion.bufferImageHeight = 0;
  bufferImageCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bufferImageCopyRegion.imageSubresource.mipLevel = 0;
  bufferImageCopyRegion.imageSubresource.baseArrayLayer = 0;
  bufferImageCopyRegion.imageSubresource.layerCount = 1;
  bufferImageCopyRegion.imageOffset.x = 0;
  bufferImageCopyRegion.imageOffset.y = 0;
  bufferImageCopyRegion.imageOffset.z = 0;
  bufferImageCopyRegion.imageExtent.width = width;
  bufferImageCopyRegion.imageExtent.height = height;
  bufferImageCopyRegion.imageExtent.depth = 1;

  vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopyRegion);
  if (!commandFinish(window, commandBuffer)) return (logError("Failed to finish single time command buffer"), false);
  return true;
}