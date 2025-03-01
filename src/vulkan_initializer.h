#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <span>
#include <unordered_set>
#include <format>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

constexpr VkExtent2D defaultWindowSize = {1280, 720};

class VulkanInitializer
{
public:
    VulkanInitializer() = default;
    ~VulkanInitializer();
    VulkanInitializer(const VulkanInitializer &) = delete;
    VulkanInitializer &operator=(const VulkanInitializer &) = delete;
    VulkanInitializer(VulkanInitializer &&) = delete;
    VulkanInitializer &operator=(VulkanInitializer &&) = delete;
    static VulkanInitializer &getInitializer()
    {
        static VulkanInitializer vkInitializer;
        return vkInitializer;
    }

    auto getInstance() const { return this->instance; }
    auto getSurface() const { return this->surface; }
    void setSurface(VkSurfaceKHR surface) { this->surface = surface; }

    void addInstanceLayer(const char *layerName) { instanceLayers.push_back(layerName); }
    void addInstanceExtension(const char *extensionName) { instanceExtensions.push_back(extensionName); }
    void addDeviceExtension(const char *extensionName) { deviceExtensions.push_back(extensionName); }
    void addCallbackCreateSwapChain(void (*function)()) { callbacksCreateSwapchain.push_back(function); }
    void addCallbackDestroySwapChain(void (*function)()) { callbacksDestroySwapchain.push_back(function); }

    VkResult deviceWaitIdle() const
    {
        VkResult result = vkDeviceWaitIdle(device);
        if (result)
            std::cout << std::format("[ graphicsBase ] ERROR\nFailed to wait for the device to be idle!\nError code: {}\n", int32_t(result));
        return result;
    }

    VkResult getLatestApiVersion()
    {
        if (vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"))
            return vkEnumerateInstanceVersion(&apiVersion);
        return VK_SUCCESS;
    }

    VkResult createInstance(VkInstanceCreateFlags flags = 0);
    VkResult getPhysicalDevices();
    VkResult pickPhysicalDevice(uint32_t deviceIndex = 0);
    VkResult createDevice(VkDeviceCreateFlags flag = 0);
    VkResult recreateDevice(VkDeviceCreateFlags flags = 0);
    VkResult getSurfaceFormats();
    VkResult setSurfaceFormat(VkSurfaceFormatKHR surfaceFormat);

    VkResult createSwapChain(bool limitFrameRate = true, VkSwapchainCreateFlagsKHR flags = 0);
    VkResult recreateSwapChain();
    VkResult createSwapChainInternal();

    VkResult checkInstanceLayers(std::span<const char *> layersToCheck) const;
    VkResult checkInstanceExtensions(std::span<const char *> extensionsToCheck, const char *layerName = nullptr) const;
    VkResult checkDeviceExtensions(std::span<const char *> extensionsToCheck, const char *layerName = nullptr) const;

    VkResult createDebugMessenger();
    void destroyDebugMessenger();

    VkResult getQueueFamilyIndices(VkPhysicalDevice physicalDevice);

private:
    uint32_t apiVersion = VK_API_VERSION_1_0;
    VkInstance instance = VK_NULL_HANDLE;
    bool enableValidationLayers = true;
    std::vector<const char *> instanceLayers;
    std::vector<const char *> instanceExtensions;
    std::vector<const char *> deviceExtensions;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties physicalDeviceProperties;
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    std::vector<VkPhysicalDevice> availablePhysicalDevices;

    VkDevice device = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex_graphics = VK_QUEUE_FAMILY_IGNORED;
    uint32_t queueFamilyIndex_presentation = VK_QUEUE_FAMILY_IGNORED;
    uint32_t queueFamilyIndex_compute = VK_QUEUE_FAMILY_IGNORED;
    VkQueue queue_graphics;
    VkQueue queue_presentation;
    VkQueue queue_compute;

    std::vector<VkSurfaceFormatKHR> availableSurfaceFormats;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    VkSwapchainKHR swapchain;

    std::vector<void (*)()> callbacksCreateSwapchain;
    std::vector<void (*)()> callbacksDestroySwapchain;
    std::vector<void (*)()> callbacksCreateDevice;
    std::vector<void (*)()> callbacksDestroyDevice;
};