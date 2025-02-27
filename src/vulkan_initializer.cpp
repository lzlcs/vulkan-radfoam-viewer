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

VkResult VulkanInitializer::createSwapChain(bool limitFrameRate, VkSwapchainCreateFlagsKHR flag)
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

    
}