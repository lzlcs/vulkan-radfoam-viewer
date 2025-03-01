#include "vulkan_initializer.h"

VkResult VulkanInitializer::createDebugMessenger()
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

    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

    if (func)
    {
        VkResult result = func(instance, &debugUtilsMessengerCreateInfo, nullptr, &debugMessenger);
        if (result)
            std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a debug messenger!\nError code: {}\n", int32_t(result));
        return result;
    }
    std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the function pointer of vkCreateDebugUtilsMessengerEXT!\n");
    return VK_RESULT_MAX_ENUM;
}

void VulkanInitializer::destroyDebugMessenger()
{
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT"));

    if (func)
    {
        func(instance, debugMessenger, nullptr);
    }
}

VkResult VulkanInitializer::createInstance(VkInstanceCreateFlags flags)
{
    if (enableValidationLayers)
    {
        addInstanceLayer("VK_LAYER_KHRONOS_validation");
        addInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

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

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS)
    {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a vulkan instance!\nError code: {}\n", int32_t(result));
        return result;
    }

    std::cout << std::format(
        "Vulkan API Version: {}.{}.{}\n",
        VK_VERSION_MAJOR(apiVersion),
        VK_VERSION_MINOR(apiVersion),
        VK_VERSION_PATCH(apiVersion));

    if (enableValidationLayers)
    {
        createDebugMessenger();
    }
    return VK_SUCCESS;
}

VkResult VulkanInitializer::getPhysicalDevices()
{
    uint32_t deviceCount = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (result != VK_SUCCESS)
    {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of physical devices!\nError code: {}\n", int32_t(result));
        return result;
    }
    if (!deviceCount)
    {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to find any physical device supports vulkan!\n");
        abort();
    }

    availablePhysicalDevices.resize(deviceCount);
    result = vkEnumeratePhysicalDevices(instance, &deviceCount, availablePhysicalDevices.data());

    if (result != VK_SUCCESS)
    {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to enumerate physical devices!\nError code: {}\n", int32_t(result));
    }
    return result;
}

VkResult VulkanInitializer::checkInstanceLayers(std::span<const char *> layersToCheck) const
{
    uint32_t layerCount;
    std::vector<VkLayerProperties> availableLayers;
    VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    if (result != VK_SUCCESS)
    {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of instance layers!\n");
        return result;
    }

    if (layerCount)
    {
        availableLayers.resize(layerCount);
        VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        if (result != VK_SUCCESS)
        {
            std::cout << std::format("[ graphicsBase ] ERROR\nFailed to enumerate instance layer properties!\nError code: {}\n", int32_t(result));
            return result;
        }
        for (auto &i : layersToCheck)
        {
            bool found = false;
            for (auto &j : availableLayers)
                if (!strcmp(i, j.layerName))
                {
                    found = true;
                    break;
                }
            if (!found)
                i = nullptr;
        }
    }
    else
        for (auto &i : layersToCheck)
            i = nullptr;

    return VK_SUCCESS;
}

VkResult VulkanInitializer::checkInstanceExtensions(std::span<const char *> extensionsToCheck, const char *layerName) const
{
    uint32_t extensionCount;
    std::vector<VkExtensionProperties> availableExtensions;
    VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr);
    if (result != VK_SUCCESS)
    {
        if (layerName)
            std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of instance extensions!\nLayer name:{}\n", layerName);
        else
            std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of instance extensions!\n");
        return result;
    }

    if (extensionCount)
    {
        availableExtensions.resize(extensionCount);
        VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, availableExtensions.data());
        if (result != VK_SUCCESS)
        {
            std::cout << std::format("[ graphicsBase ] ERROR\nFailed to enumerate instance extension properties!\nError code: {}\n", int32_t(result));
            return result;
        }
        for (auto &i : extensionsToCheck)
        {
            bool found = false;
            for (auto &j : availableExtensions)
                if (!strcmp(i, j.extensionName))
                {
                    found = true;
                    break;
                }
            if (!found)
                i = nullptr;
        }
    }
    else
        for (auto &i : extensionsToCheck)
            i = nullptr;
    return VK_SUCCESS;
}

VkResult VulkanInitializer::checkDeviceExtensions(std::span<const char *> extensionsToCheck, const char *layerName) const
{
    return VK_SUCCESS;
}

VkResult VulkanInitializer::getQueueFamilyIndices(VkPhysicalDevice physicalDevice)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    if (!queueFamilyCount)
        return VK_RESULT_MAX_ENUM;
    std::vector<VkQueueFamilyProperties> queueFamilyPropertieses(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyPropertieses.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        VkBool32 supportGraphics = (queueFamilyPropertieses[i].queueFlags & VK_QUEUE_GRAPHICS_BIT);
        VkBool32 supportPresentation = false;
        VkBool32 supportCompute = (queueFamilyPropertieses[i].queueFlags & VK_QUEUE_COMPUTE_BIT);

        if (surface)
        {
            VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportPresentation);
            if (result != VK_SUCCESS)
            {
                std::cout << std::format("[ graphicsBase ] ERROR\nFailed to determine if the queue family supports presentation!\nError code: {}\n", int32_t(result));
                return result;
            }
        }

        if (supportGraphics && supportCompute &&
            (!surface || supportPresentation))
        {
            queueFamilyIndex_graphics = queueFamilyIndex_compute = i;
            if (surface)
                queueFamilyIndex_presentation = i;
            return VK_SUCCESS;
        }
    }

    return VK_SUCCESS;
}

VkResult VulkanInitializer::pickPhysicalDevice(uint32_t deviceIndex)
{
    static constexpr uint32_t notFound = INT32_MAX;
    static std::vector<uint32_t> queueFamilyIndices(availablePhysicalDevices.size());

    if (queueFamilyIndices[deviceIndex] == notFound)
        return VK_RESULT_MAX_ENUM;

    if (queueFamilyIndices[deviceIndex] == VK_QUEUE_FAMILY_IGNORED)
    {
        VkResult result = getQueueFamilyIndices(availablePhysicalDevices[deviceIndex]);

        if (result)
        {
            if (result == VK_RESULT_MAX_ENUM)
                queueFamilyIndices[deviceIndex] = notFound;
            return result;
        }
        else
            queueFamilyIndices[deviceIndex] = queueFamilyIndex_graphics;
    }
    else
    {
        queueFamilyIndex_graphics = queueFamilyIndex_compute = queueFamilyIndices[deviceIndex];
        queueFamilyIndex_presentation = surface ? queueFamilyIndices[deviceIndex] : VK_QUEUE_FAMILY_IGNORED;
    }
    physicalDevice = availablePhysicalDevices[deviceIndex];

    return VK_SUCCESS;
}

VkResult VulkanInitializer::createDevice(VkDeviceCreateFlags flags)
{
    // Device Queue Create Info
    float queuePriority = 1.f;
    VkDeviceQueueCreateInfo queueCreateInfos[3]{{}, {}, {}};
    auto queueCreateFunc = [&queueCreateInfos, &queuePriority](uint32_t idx)
    {
        queueCreateInfos[idx].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[idx].queueCount = 1;
        queueCreateInfos[idx].pQueuePriorities = &queuePriority;
    };
    for (int i = 0; i < 3; i++)
        queueCreateFunc(i);
    uint32_t queueCreateInfoCount = 0;
    if (queueFamilyIndex_graphics != VK_QUEUE_FAMILY_IGNORED)
        queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_graphics;
    if (queueFamilyIndex_presentation != VK_QUEUE_FAMILY_IGNORED &&
        queueFamilyIndex_presentation != queueFamilyIndex_graphics)
        queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_presentation;
    if (queueFamilyIndex_compute != VK_QUEUE_FAMILY_IGNORED &&
        queueFamilyIndex_compute != queueFamilyIndex_graphics &&
        queueFamilyIndex_compute != queueFamilyIndex_presentation)
        queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_compute;

    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

    // Create Logical Device
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.flags = flags;
    deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    deviceCreateInfo.enabledExtensionCount = uint32_t(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
    VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);

    if (result != VK_SUCCESS)
    {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a vulkan logical device!\nError code: {}\n", int32_t(result));
        return result;
    }

    if (queueFamilyIndex_graphics != VK_QUEUE_FAMILY_IGNORED)
        vkGetDeviceQueue(device, queueFamilyIndex_graphics, 0, &queue_graphics);
    if (queueFamilyIndex_presentation != VK_QUEUE_FAMILY_IGNORED)
        vkGetDeviceQueue(device, queueFamilyIndex_presentation, 0, &queue_presentation);
    if (queueFamilyIndex_compute != VK_QUEUE_FAMILY_IGNORED)
        vkGetDeviceQueue(device, queueFamilyIndex_compute, 0, &queue_compute);

    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);
    std::cout << std::format("Renderer: {}\n", physicalDeviceProperties.deviceName);

    return VK_SUCCESS;
}

VkResult VulkanInitializer::createSwapChain(bool limitFrameRate, VkSwapchainCreateFlagsKHR flags)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
    if (result != VK_SUCCESS)
    {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: {}\n", int32_t(result));
        return result;
    }
    // Set Image Extent
    swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + (surfaceCapabilities.maxImageCount > surfaceCapabilities.minImageCount);
    swapchainCreateInfo.imageExtent =
        surfaceCapabilities.currentExtent.width == -1 ? VkExtent2D{glm::clamp(defaultWindowSize.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
                                                                   glm::clamp(defaultWindowSize.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)}
                                                      : surfaceCapabilities.currentExtent;

    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;

    if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    else
        for (size_t i = 0; i < 4; i++)
            if (surfaceCapabilities.supportedCompositeAlpha & 1 << i)
            {
                swapchainCreateInfo.compositeAlpha = VkCompositeAlphaFlagBitsKHR(surfaceCapabilities.supportedCompositeAlpha & 1 << i);
                break;
            }
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    else
        std::cout << std::format("[ graphicsBase ] WARNING\nVK_IMAGE_USAGE_TRANSFER_DST_BIT isn't supported!\n");

    // Get surface formats
    if (!availableSurfaceFormats.size())
        if (VkResult result = getSurfaceFormats())
            return result;
    // If surface format is not determined, select a a four-component UNORM format
    if (!swapchainCreateInfo.imageFormat)
        if (setSurfaceFormat({VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}) &&
            setSurfaceFormat({VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}))
        {
            swapchainCreateInfo.imageFormat = availableSurfaceFormats[0].format;
            swapchainCreateInfo.imageColorSpace = availableSurfaceFormats[0].colorSpace;
            std::cout << std::format("[ graphicsBase ] WARNING\nFailed to select a four-component UNORM surface format!\n");
        }

    // Get surface present modes
    uint32_t surfacePresentModeCount;
    if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, nullptr))
    {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of surface present modes!\nError code: {}\n", int32_t(result));
        return result;
    }
    if (!surfacePresentModeCount)
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to find any surface present mode!\n"),
            abort();

    std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
    if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, surfacePresentModes.data()))
    {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get surface present modes!\nError code: {}\n", int32_t(result));
        return result;
    }
    // Set present mode to mailbox if available and necessary
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (!limitFrameRate)
        for (size_t i = 0; i < surfacePresentModeCount; i++)
            if (surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }

    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.flags = flags;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.clipped = VK_TRUE;

    if (VkResult result = createSwapChainInternal())
        return result;

    for (auto &callbackCreateSwapChain : callbacksCreateSwapchain)
        callbackCreateSwapChain();
    return VK_SUCCESS;
}

VkResult VulkanInitializer::recreateSwapChain()
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
    if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities))
    {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: {}\n", int32_t(result));
        return result;
    }
    if (surfaceCapabilities.currentExtent.width == 0 ||
        surfaceCapabilities.currentExtent.height == 0)
        return VK_SUBOPTIMAL_KHR;
    swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    swapchainCreateInfo.oldSwapchain = swapchain;

    VkResult result = vkQueueWaitIdle(queue_graphics);
    if (!result &&
        queue_graphics != queue_presentation)
        result = vkQueueWaitIdle(queue_presentation);
    if (result)
    {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to wait for the queue to be idle!\nError code: {}\n", int32_t(result));
        return result;
    }

    for (auto &callbackDestroySwapChain : callbacksDestroySwapchain)
        callbackDestroySwapChain();

    for (auto &i : swapchainImageViews)
        if (i)
            vkDestroyImageView(device, i, nullptr);

    swapchainImageViews.resize(0);

    if (result = createSwapChainInternal())
        return result;
    for (auto &callbackCreateSwapChain : callbacksCreateSwapchain)
        callbackCreateSwapChain();
    return VK_SUCCESS;
}

VkResult VulkanInitializer::createSwapChainInternal()
{
    if (VkResult result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain))
    {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a swapchain!\nError code: {}\n", int32_t(result));
        return result;
    }

    uint32_t swapchainImageCount;
    if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr))
    {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of swapchain images!\nError code: {}\n", int32_t(result));
        return result;
    }
    swapchainImages.resize(swapchainImageCount);
    if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data()))
    {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get swapchain images!\nError code: {}\n", int32_t(result));
        return result;
    }

    swapchainImageViews.resize(swapchainImageCount);
    VkImageViewCreateInfo imageViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapchainCreateInfo.imageFormat,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    for (size_t i = 0; i < swapchainImageCount; i++)
    {
        imageViewCreateInfo.image = swapchainImages[i];
        if (VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]))
        {
            std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a swapchain image view!\nError code: {}\n", int32_t(result));
            return result;
        }
    }
    return VK_SUCCESS;
}

VkResult VulkanInitializer::getSurfaceFormats()
{
    uint32_t surfaceFormatCount;
    if (VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr))
    {
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of surface formats!\nError code: {}\n", int32_t(result));
        return result;
    }
    if (!surfaceFormatCount)
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to find any supported surface format!\n"),
            abort();
    availableSurfaceFormats.resize(surfaceFormatCount);
    VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, availableSurfaceFormats.data());
    if (result)
        std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get surface formats!\nError code: {}\n", int32_t(result));
    return result;
}

VkResult VulkanInitializer::setSurfaceFormat(VkSurfaceFormatKHR surfaceFormat)
{
    bool formatIsAvailable = false;
    if (!surfaceFormat.format)
    {
        for (auto &i : availableSurfaceFormats)
            if (i.colorSpace == surfaceFormat.colorSpace)
            {
                swapchainCreateInfo.imageFormat = i.format;
                swapchainCreateInfo.imageColorSpace = i.colorSpace;
                formatIsAvailable = true;
                break;
            }
    }
    else
        for (auto &i : availableSurfaceFormats)
            if (i.format == surfaceFormat.format &&
                i.colorSpace == surfaceFormat.colorSpace)
            {
                swapchainCreateInfo.imageFormat = i.format;
                swapchainCreateInfo.imageColorSpace = i.colorSpace;
                formatIsAvailable = true;
                break;
            }
    if (!formatIsAvailable)
        return VK_ERROR_FORMAT_NOT_SUPPORTED;
    if (swapchain)
        return recreateSwapChain();
    return VK_SUCCESS;
}

VulkanInitializer::~VulkanInitializer()
{

    if (!instance)
        return;
    if (device)
    {
        deviceWaitIdle();
        if (swapchain)
        {
            for (auto &i : callbacksDestroySwapchain)
                i();
            for (auto &i : swapchainImageViews)
                if (i)
                    vkDestroyImageView(device, i, nullptr);
            vkDestroySwapchainKHR(device, swapchain, nullptr);
        }
        for (auto &i : callbacksDestroyDevice)
            i();
        vkDestroyDevice(device, nullptr);
    }
    if (surface)
        vkDestroySurfaceKHR(instance, surface, nullptr);
    if (debugMessenger)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessenger =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (DestroyDebugUtilsMessenger)
            DestroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
}

VkResult VulkanInitializer::recreateDevice(VkDeviceCreateFlags flags) {
    if (VkResult result = deviceWaitIdle())
        return result;

    if (swapchain) {
        for (auto& i : callbacksDestroySwapchain)
            i();
        for (auto& i : swapchainImageViews)
            if (i)
                vkDestroyImageView(device, i, nullptr);
        swapchainImageViews.resize(0);
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
        swapchainCreateInfo = {};
    }
    for (auto& i : callbacksDestroyDevice)
        i();
    if (device)
        vkDestroyDevice(device, nullptr),
        device = VK_NULL_HANDLE;
    return createDevice(flags);
}