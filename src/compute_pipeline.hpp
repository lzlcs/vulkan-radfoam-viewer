#pragma once
#include <string>
#include "vulkan_context.h"

class Shader
{
public:
    Shader(const std::string &filePath);
    ~Shader();
    VkShaderModule shaderModule;
};

class DescriptorSet
{
public:
    struct BindingInfo
    {
        uint32_t binding;
        VkDescriptorType type;
        uint32_t descriptorCount;
        VkShaderStageFlags stageFlags;
    };
    DescriptorSet(const std::vector<BindingInfo> &bindings) : bindings(bindings)
    {
        createDescriptorLayout();
    }

    ~DescriptorSet()
    {
        vkDestroyDescriptorSetLayout(VulkanContext::getContext().getDevice(), layout, nullptr);
    }

    void BindBuffers(uint32_t binding, const std::vector<VkBuffer> &buffers,
                     VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE,
                     uint32_t startArrayIndex = 0)
    {
        auto &bindingInfo = getBindingInfo(binding);
        assert(buffers.size() == bindingInfo.descriptorCount && "Error binding");

        std::vector<VkDescriptorBufferInfo> bufferInfos;
        for (auto buffer : buffers)
        {
            bufferInfos.push_back({buffer, offset, range});
        }

        VkWriteDescriptorSet write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = set,
            .dstBinding = binding,
            .dstArrayElement = startArrayIndex,
            .descriptorCount = static_cast<uint32_t>(buffers.size()),
            .descriptorType = bindingInfo.type,
            .pBufferInfo = bufferInfos.data()};
        vkUpdateDescriptorSets(VulkanContext::getContext().getDevice(), 1, &write, 0, nullptr);
    }

private:
    void createDescriptorLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        for (const auto &binding : bindings)
        {
            layoutBindings.push_back({.binding = binding.binding,
                                      .descriptorType = binding.type,
                                      .descriptorCount = binding.descriptorCount,
                                      .stageFlags = binding.stageFlags});
        }
        VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
            .pBindings = layoutBindings.data()};
        vkCreateDescriptorSetLayout(VulkanContext::getContext().getDevice(), &layoutInfo, nullptr, &layout);
    }

    const BindingInfo &getBindingInfo(uint32_t binding) const
    {
        for (const auto &b : bindings)
        {
            if (b.binding == binding)
                return b;
        }
        throw std::invalid_argument("Invalid binding index!");
    }

    std::vector<BindingInfo> bindings;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;
};