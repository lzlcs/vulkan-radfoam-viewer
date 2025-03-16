#include "src/vulkan_context.h"
#include "src/GLFWGeneral.h"
#include "src/arguments.hpp"
#include "src/renderer.hpp"


int main(int argc, char *argv[])
{
    auto args = argparse::parse<RadFoamVulkanArgs>(argc, argv);
    auto pArgs = std::make_shared<RadFoamVulkanArgs>(args);

    initializeWindow(pArgs);
    auto pModel = std::make_shared<RadFoam>(pArgs);
    auto pAABB = std::make_shared<AABBTree>(pModel);

    // std::cout << pAABB->aabbTree[(1 << pAABB->numLevels) - 2].min[0] << std::endl;
    // std::cout << pAABB->aabbTree[(1 << pAABB->numLevels) - 2].min[1] << std::endl;
    // std::cout << pAABB->aabbTree[(1 << pAABB->numLevels) - 2].min[2] << std::endl;
    // std::cout << pAABB->aabbTree[(1 << pAABB->numLevels) - 2].max[0] << std::endl;
    // std::cout << pAABB->aabbTree[(1 << pAABB->numLevels) - 2].max[1] << std::endl;
    // std::cout << pAABB->aabbTree[(1 << pAABB->numLevels) - 2].max[2] << std::endl;

    auto renderer = std::make_shared<Renderer>(pArgs, pModel, pAABB);
    
        
    // renderer->render();

    while (!glfwWindowShouldClose(pWindow))
    {
        glfwPollEvents();
        renderer->render();
        TitleFps();
    }   

    terminateWindow();
    return 0;
}
