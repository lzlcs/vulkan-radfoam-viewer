#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <span>
#include <unordered_set>
#include <format>
#include "arguments.hpp"

class RadFoam;

class VulkanContext
{
public:
    VulkanContext() = default;
    ~VulkanContext();
    VulkanContext(const VulkanContext &) = delete;
    VulkanContext &operator=(const VulkanContext &) = delete;
    VulkanContext(VulkanContext &&) = delete;
    VulkanContext &operator=(VulkanContext &&) = delete;
    static VulkanContext &getContext()
    {
        static VulkanContext vkContext;
        return vkContext;
    }

    auto getArgs() { return this->pArgs; }
    auto getInstance() const { return this->instance; }
    auto getSurface() const { return this->surface; }
    auto getDevice() const { return this->device; }
    // auto getModel() const { return this->pModel; }
    auto getAllocator() const { return this->allocator; }
    auto getDescriptorPool() const { return this->descriptorPool; }
    auto getCommandPool() const { return this->commandPool; }
    auto getSwapChain() const { return this->swapchain; }
    auto getSwapChainImage(uint32_t idx) { return this->swapchainImages[idx]; }

    VkQueue getQueue(const std::string &type)
    {
        if (type == "compute") return queue_compute;
        else if (type == "graphics") return queue_graphics;
        else if (type == "present") return queue_presentation;
        return VK_NULL_HANDLE;
    }

    void setArgs(auto pArgs) { this->pArgs = pArgs; }
    // void setModel(std::shared_ptr<RadFoam> pModel) { this->pModel = pModel; }
    void setSurface(VkSurfaceKHR surface) { this->surface = surface; }

    void addInstanceLayer(const char *layerName) { instanceLayers.push_back(layerName); }
    void addInstanceExtension(const char *extensionName) { instanceExtensions.push_back(extensionName); }
    void addDeviceExtension(const char *extensionName) { deviceExtensions.push_back(extensionName); }
    void addCallbackCreateSwapChain(void (*function)()) { callbacksCreateSwapchain.push_back(function); }
    void addCallbackDestroySwapChain(void (*function)()) { callbacksDestroySwapchain.push_back(function); }

    void createInstance(VkInstanceCreateFlags flags = 0);
    void getPhysicalDevices(uint32_t deviceIndex = 0);
    void createLogicalDevice(VkDeviceCreateFlags flag = 0);
    void createVMAAllocator();
    void createCommandPool();
    void createDescriptorSetPool();
    void createSwapChain(VkSwapchainCreateFlagsKHR flags = 0);
    void recreateSwapChain();
    void createSwapChainInternal();
    void createDebugMessenger();
    void destroyDebugMessenger();

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

private:
    std::shared_ptr<RadFoamVulkanArgs> pArgs;
    // std::shared_ptr<RadFoam> pModel;
    VmaAllocator allocator;

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

    VkCommandPool commandPool;
    VkDescriptorPool descriptorPool;

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