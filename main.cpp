#include "src/vulkan_context.h"
#include "src/GLFWGeneral.h"
#include "src/arguments.hpp"


int main(int argc, char *argv[])
{
    auto args = argparse::parse<RadFoamVulkanArgs>(argc, argv);
    auto pArgs = std::make_shared<RadFoamVulkanArgs>(args);

    initializeWindow(pArgs);
        
    while (!glfwWindowShouldClose(pWindow))
    {

        /*渲染过程，待填充*/

        glfwPollEvents();
        TitleFps();
    }

    terminateWindow();
    return 0;
}
