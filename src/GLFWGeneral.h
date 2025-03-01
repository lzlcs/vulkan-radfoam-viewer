#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "vulkan_initializer.h"
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

bool InitializeWindow(VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true)
{
    auto &initializer = VulkanInitializer::getInitializer();

    // Initialize Window
    if (!glfwInit())
    {
        std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to initialize GLFW!\n");
        return false;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, isResizable);
    pMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *pMode = glfwGetVideoMode(pMonitor);
    pWindow = fullScreen ? glfwCreateWindow(pMode->width, pMode->height, windowTitle, pMonitor, nullptr) : glfwCreateWindow(size.width, size.height, windowTitle, nullptr, nullptr);
    if (!pWindow)
    {
        std::cout << std::format("[ InitializeWindow ]\nFailed to create a glfw window!\n");
        glfwTerminate();
        return false;
    }

    // Enable Extensions
    uint32_t extensionCount = 0;
    const char **extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);

    if (!extensionNames)
    {
        std::cout << std::format("[ InitializeWindow ]\nVulkan is not available on this machine!\n");
        glfwTerminate();
        return false;
    }
    for (size_t i = 0; i < extensionCount; i++)
        initializer.addInstanceExtension(extensionNames[i]);

    initializer.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    initializer.getLatestApiVersion();
    if (initializer.createInstance())
        return false;

    // Create SurfaceKHR
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkResult result = glfwCreateWindowSurface(initializer.getInstance(), pWindow, nullptr, &surface);
    if (result != VK_SUCCESS)
    {
        std::cout << std::format("[ InitializeWindow ] ERROR\nFailed to create a window surface!\nError code: {}\n", int32_t(result));
        glfwTerminate();
        return false;
    }
    initializer.setSurface(surface);

    // Create Device
    if (initializer.getPhysicalDevices() != VK_SUCCESS ||
        initializer.pickPhysicalDevice() != VK_SUCCESS ||
        initializer.createDevice() != VK_SUCCESS)
    {
        return false;
    }

    if (initializer.createSwapChain(limitFrameRate))
        return false;

    return true;
}
void TerminateWindow()
{
    VulkanInitializer::getInitializer().deviceWaitIdle();
    glfwTerminate();
}