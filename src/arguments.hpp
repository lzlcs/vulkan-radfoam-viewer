#pragma once
#include "VmaUsage.hpp"

#include <argparse/argparse.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/integer.hpp>

#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)
#define LINE_STRING STRINGIZE(__LINE__)
#define ERR_GUARD_VULKAN(expr)  do { if((expr) != 0) { \
            assert(0 && #expr); \
            throw std::runtime_error(__FILE__ "(" LINE_STRING "): VkResult( " #expr " ) < 0"); \
        } } while(false)


struct RadFoamVulkanArgs : public argparse::Args
{
    std::string &scenePath = arg("path to radfoam's output ply file");
    bool &validation = flag("validation", "enable vulkan vadilation layer");
    uint32_t &windowWidth = kwarg("width", "Init Window Width").set_default(1280u);
    uint32_t &windowHeight = kwarg("height", "Init Window Height").set_default(960u);
    bool &fullScreen = flag("fullScreen", "enable full screen window");
    bool &isResizable = flag("resizable", "enable resizable window");
    // bool &limitFrameRate = flag("limitFrameRate", "enable limit frame rate");
};
