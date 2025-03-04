#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "vulkan_context.h"
#include "arguments.hpp"
#include "radfoam.hpp"
#include <iostream>
#include <sstream>
#include <format>

GLFWwindow *pWindow;
GLFWmonitor *pMonitor;
const char *windowTitle = "RadFoam Vulakn Viewer";

void TitleFps()
{
    static double time0 = glfwGetTime();
    static double time1;
    static double dt;
    static int dframe = -1;
    static std::stringstream info;
    time1 = glfwGetTime();
    dframe++;
    if ((dt = time1 - time0) >= 1)
    {
        info.precision(1);
        info << windowTitle << "    " << std::fixed << dframe / dt << " FPS";
        glfwSetWindowTitle(pWindow, info.str().c_str());
        info.str("");
        time0 = time1;
        dframe = 0;
    }
}

VkResult createGLFW(std::shared_ptr<RadFoamVulkanArgs> pArgs)
{
    // Initialize Window
    if (!glfwInit())
    {
        glfwTerminate();
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, pArgs->isResizable);
    pMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *pMode = glfwGetVideoMode(pMonitor);

    pWindow = pArgs->fullScreen ? glfwCreateWindow(pMode->width, pMode->height, windowTitle, pMonitor, nullptr)
                                : glfwCreateWindow(pArgs->windowWidth, pArgs->windowHeight, windowTitle, nullptr, nullptr);
    if (!pWindow)
    {
        glfwTerminate();
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    return VK_SUCCESS;
}

VkResult getGLFWInstanceExtensions(uint32_t *extensionCount, const char ***extensionNames)
{
    *extensionNames = glfwGetRequiredInstanceExtensions(extensionCount);
    if (!*extensionNames)
    {
        glfwTerminate();
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    return VK_SUCCESS;
}

void initializeWindow(std::shared_ptr<RadFoamVulkanArgs> pArgs)
{
    
    ERR_GUARD_VULKAN(createGLFW(pArgs));
    auto &context = VulkanContext::getContext();
    context.setArgs(pArgs);

    // Add Instance Layers, Instance Extensions, Device Extensions
    uint32_t extensionCount = 0;
    const char **extensionNames = nullptr;
    ERR_GUARD_VULKAN(getGLFWInstanceExtensions(&extensionCount, &extensionNames));
    for (size_t i = 0; i < extensionCount; i++)
        context.addInstanceExtension(extensionNames[i]);
    if (pArgs->validation)
    {
        context.addInstanceLayer("VK_LAYER_KHRONOS_validation");
        context.addInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    context.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    // Initialization
    context.createInstance();

    if (pArgs->validation)
        context.createDebugMessenger();

    context.getPhysicalDevices();
    context.createLogicalDevice();
    context.createCommandPool();
    context.createVMAAllocator();
    context.setModel(std::make_shared<RadFoam>(pArgs));
    context.getModel()->loadRadFoam();
    
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    ERR_GUARD_VULKAN(glfwCreateWindowSurface(context.getInstance(), pWindow, nullptr, &surface));
    context.setSurface(surface);

    context.createSwapChain();
}

void terminateWindow()
{
    ERR_GUARD_VULKAN(vkDeviceWaitIdle(VulkanContext::getContext().getDevice()));
    glfwTerminate();
}