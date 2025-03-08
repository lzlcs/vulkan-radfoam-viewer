#include "compute_pipeline.hpp"
#include "vulkan_context.h"
#include <fstream>

std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}


Shader::Shader(const std::string &filePath)
{
    auto &context = VulkanContext::getContext();
    std::vector<char> shaderCode = readFile("tree_builder.comp.spv");

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    ERR_GUARD_VULKAN(vkCreateShaderModule(context.getDevice(), &createInfo, nullptr, &shaderModule));
}

Shader::~Shader()
{
    vkDestroyShaderModule(VulkanContext::getContext().getDevice(), shaderModule, nullptr);
}