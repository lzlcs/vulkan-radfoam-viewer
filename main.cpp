#include "src/vulkan_initializer.h"
#include "src/GLFWGeneral.h"


int main()
{
    VulkanInitializer vkInitializer;
    if (!InitializeWindow({1280, 720}))
        return -1;
        
    while (!glfwWindowShouldClose(pWindow))
    {

        /*渲染过程，待填充*/

        glfwPollEvents();
        TitleFps();
    }
    TerminateWindow();
    return 0;
}
