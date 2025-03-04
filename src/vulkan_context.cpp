#include "vulkan_context.h"
#include "radfoam.hpp"

void VulkanContext::createDebugMessenger()
{
    static PFN_vkDebugUtilsMessengerCallbackEXT DebugUtilsMessengerCallback =
        [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
           VkDebugUtilsMessageTypeFlagsEXT messageTypes,
           const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
           void *pUserData) -> VkBool32
    {
        std::cout << std::format("{}\n\n", pCallbackData->pMessage);
        return VK_FALSE;
    };

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{};
    debugUtilsMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugUtilsMessengerCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugUtilsMessengerCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugUtilsMessengerCreateInfo.pfnUserCallback = DebugUtilsMessengerCallback;

    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    assert(func != nullptr && "Cannot get PFN_vkCreateDebugUtilsMessengerEXT");
    ERR_GUARD_VULKAN(func(instance, &debugUtilsMessengerCreateInfo, nullptr, &debugMessenger));
}

void VulkanContext::destroyDebugMessenger()
{
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    assert(func != nullptr && "Cannot get PFN_vkDestroyDebugUtilsMessengerEXT");
    func(instance, debugMessenger, nullptr);
}

void VulkanContext::createInstance(VkInstanceCreateFlags flags)
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "RadFoam Viewer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
    createInfo.ppEnabledLayerNames = instanceLayers.data();

    ERR_GUARD_VULKAN(vkCreateInstance(&createInfo, nullptr, &instance));
}

void VulkanContext::getPhysicalDevices(uint32_t deviceIndex)
{
    uint32_t deviceCount = 0;
    ERR_GUARD_VULKAN(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
    assert(deviceCount != 0 && "Failed to find any physical device supports vulkan!");

    std::vector<VkPhysicalDevice> availablePhysicalDevices(deviceCount);
    ERR_GUARD_VULKAN(vkEnumeratePhysicalDevices(instance, &deviceCount, availablePhysicalDevices.data()));
    physicalDevice = availablePhysicalDevices[deviceIndex];
}

void VulkanContext::createLogicalDevice(VkDeviceCreateFlags flags)
{
    // Get Device Queue Families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    assert(queueFamilyCount != 0 && "queue family count == 0");
    std::vector<VkQueueFamilyProperties> queueFamilyPropertieses(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyPropertieses.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        VkBool32 supportGraphics = (queueFamilyPropertieses[i].queueFlags & VK_QUEUE_GRAPHICS_BIT);
        VkBool32 supportPresentation = false;
        VkBool32 supportCompute = (queueFamilyPropertieses[i].queueFlags & VK_QUEUE_COMPUTE_BIT);

        if (surface)
        {
            ERR_GUARD_VULKAN(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportPresentation));
        }

        if (supportGraphics && supportCompute &&
            (!surface || supportPresentation))
        {
            queueFamilyIndex_graphics = queueFamilyIndex_compute = i;
            if (surface)
                queueFamilyIndex_presentation = i;
            break;
        }
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex_graphics;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    ERR_GUARD_VULKAN(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));

    vkGetDeviceQueue(device, queueFamilyIndex_graphics, 0, &queue_graphics);
    queue_compute = queue_graphics;
    queue_presentation = (surface != VK_NULL_HANDLE) ? queue_graphics : VK_NULL_HANDLE;

    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);
    std::cout << std::format("Renderer: {}\n", physicalDeviceProperties.deviceName);
}

void VulkanContext::createVMAAllocator()
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    vmaCreateAllocator(&allocatorInfo, &allocator);
}

void VulkanContext::createCommandPool()
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex_graphics;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ERR_GUARD_VULKAN(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS);
}

void VulkanContext::createSwapChain(VkSwapchainCreateFlagsKHR flags)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    ERR_GUARD_VULKAN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities));

    swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + (surfaceCapabilities.maxImageCount > surfaceCapabilities.minImageCount);
    if (surfaceCapabilities.currentExtent.width == UINT32_MAX)
    {
        swapchainCreateInfo.imageExtent = VkExtent2D{glm::clamp(pArgs->windowWidth,
                                                                surfaceCapabilities.minImageExtent.width,
                                                                surfaceCapabilities.maxImageExtent.width),
                                                     glm::clamp(pArgs->windowHeight,
                                                                surfaceCapabilities.minImageExtent.height,
                                                                surfaceCapabilities.maxImageExtent.height)};
    }
    else
    {
        swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    }

    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    swapchainCreateInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.flags = flags;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.clipped = VK_TRUE;

    createSwapChainInternal();

    for (auto &callbackCreateSwapChain : callbacksCreateSwapchain)
        callbackCreateSwapChain();
}

void VulkanContext::recreateSwapChain()
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    ERR_GUARD_VULKAN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities));
    assert(surfaceCapabilities.currentExtent.width != 0 && surfaceCapabilities.currentExtent.height != 0);

    swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    swapchainCreateInfo.oldSwapchain = swapchain;

    VkResult result = vkQueueWaitIdle(queue_graphics);
    if (!result && queue_presentation != VK_NULL_HANDLE)
        result = vkQueueWaitIdle(queue_presentation);
    ERR_GUARD_VULKAN(result);

    for (auto &callbackDestroySwapChain : callbacksDestroySwapchain)
        callbackDestroySwapChain();

    for (auto &i : swapchainImageViews)
        if (i)
            vkDestroyImageView(device, i, nullptr);

    swapchainImageViews.resize(0);
    createSwapChainInternal();

    for (auto &callbackCreateSwapChain : callbacksCreateSwapchain)
        callbackCreateSwapChain();
}

void VulkanContext::createSwapChainInternal()
{
    ERR_GUARD_VULKAN(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain));

    uint32_t swapchainImageCount;
    ERR_GUARD_VULKAN(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr));
    swapchainImages.resize(swapchainImageCount);
    ERR_GUARD_VULKAN(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data()));

    swapchainImageViews.resize(swapchainImageCount);
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = swapchainCreateInfo.imageFormat;
    imageViewCreateInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    for (size_t i = 0; i < swapchainImageCount; i++)
    {
        imageViewCreateInfo.image = swapchainImages[i];
        ERR_GUARD_VULKAN(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]));
    }
}

VulkanContext::~VulkanContext()
{

    if (!instance)
        return;
    if (device)
    {
        vkDeviceWaitIdle(device);

        if (swapchain)
        {
            for (auto &i : callbacksDestroySwapchain)
                i();
            for (auto &i : swapchainImageViews)
                if (i)
                    vkDestroyImageView(device, i, nullptr);
            vkDestroySwapchainKHR(device, swapchain, nullptr);
        }
        pModel->unloadRadFoam();
        vmaDestroyAllocator(allocator);
        vkDestroyCommandPool(device, commandPool, nullptr);

        for (auto &i : callbacksDestroyDevice)
            i();
        vkDestroyDevice(device, nullptr);
    }
    if (surface)
        vkDestroySurfaceKHR(instance, surface, nullptr);
    if (debugMessenger)
        destroyDebugMessenger();
    vkDestroyInstance(instance, nullptr);
}

VkCommandBuffer VulkanContext::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    ERR_GUARD_VULKAN(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    return commandBuffer;
}

void VulkanContext::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue_graphics, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue_graphics); 

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}