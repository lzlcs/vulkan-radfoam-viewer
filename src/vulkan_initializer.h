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

constexpr VkExtent2D defaultWindowSize = { 1280, 720 };

class VulkanInitializer
{
public:
    VulkanInitializer() = default;
    // ~VulkanInitializer();
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
    auto setSurface(VkSurfaceKHR surface) { this->surface = surface; }

    void addInstanceLayer(const char *layerName) { instanceLayers.push_back(layerName); }
    void addInstanceExtension(const char *extensionName) { instanceExtensions.push_back(extensionName); }
    void addDeviceExtension(const char *extensionName) { deviceExtensions.push_back(extensionName); }
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
    VkResult createSwapChain(bool limitFrameRate = true, VkSwapchainCreateFlagsKHR flags = 0);

    VkResult checkInstanceLayers(std::span<const char *> layersToCheck) const;
    VkResult checkInstanceExtensions(std::span<const char *> extensionsToCheck, const char *layerName = nullptr) const;
    VkResult checkDeviceExtensions(std::span<const char *> extensionsToCheck, const char *layerName = nullptr) const;

    VkResult createDebugMessenger();
    void destroyDebugMessenger();

    VkResult getQueueFamilyIndices(VkPhysicalDevice physicalDevice);

private:
    uint32_t apiVersion = VK_API_VERSION_1_0;
    VkInstance instance;
    bool enableValidationLayers = true;
    // const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    // const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    std::vector<const char *> instanceLayers;
    std::vector<const char *> instanceExtensions;
    std::vector<const char *> deviceExtensions;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties physicalDeviceProperties;
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    std::vector<VkPhysicalDevice> availablePhysicalDevices;

    VkDevice device;
    uint32_t queueFamilyIndex_graphics = VK_QUEUE_FAMILY_IGNORED;
    uint32_t queueFamilyIndex_presentation = VK_QUEUE_FAMILY_IGNORED;
    uint32_t queueFamilyIndex_compute = VK_QUEUE_FAMILY_IGNORED;
    VkQueue queue_graphics;
    VkQueue queue_presentation;
    VkQueue queue_compute;

    std::vector<VkSurfaceFormatKHR> availableSurfaceFormats;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
};