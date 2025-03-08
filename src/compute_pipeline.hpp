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
        auto &context = VulkanContext::getContext();

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
        vkCreateDescriptorSetLayout(context.getDevice(), &layoutInfo, nullptr, &layout);

        VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = context.getDescriptorPool(),
            .descriptorSetCount = 1,
            .pSetLayouts = &layout};

        vkAllocateDescriptorSets(VulkanContext::getContext().getDevice(), &allocInfo, &set);
    }

    ~DescriptorSet()
    {
        vkDestroyDescriptorSetLayout(VulkanContext::getContext().getDevice(), layout, nullptr);
    }

    void bindBuffers(uint32_t binding, const std::vector<VkBuffer> &buffers,
                     VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE,
                     uint32_t startArrayIndex = 0)
    {
        auto &bindingInfo = getBindingInfo(binding);

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

    auto getDescriptorSetLayout() const { return this->layout; }
    auto getDescriptorSet() const { return this->set; }

private:
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

class ComputePipeline
{
public:
    ComputePipeline(
        VkShaderModule computeShader,
        const std::vector<VkDescriptorSetLayout> &descriptorSetLayouts,
        const std::vector<VkPushConstantRange> &pushConstantRanges)
    {
        auto &context = VulkanContext::getContext();
        VkPipelineLayoutCreateInfo layoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
            .pSetLayouts = descriptorSetLayouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
            .pPushConstantRanges = pushConstantRanges.data()};
        ERR_GUARD_VULKAN(vkCreatePipelineLayout(context.getDevice(), &layoutInfo, nullptr, &layout));

        VkPipelineShaderStageCreateInfo shaderStageInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = computeShader,
            .pName = "main"};

        VkComputePipelineCreateInfo pipelineInfo{
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = shaderStageInfo,
            .layout = layout};
        ERR_GUARD_VULKAN(vkCreateComputePipelines(
            context.getDevice(), VK_NULL_HANDLE,
            1, &pipelineInfo, nullptr, &pipeline));
    }

    ~ComputePipeline()
    {
        auto device = VulkanContext::getContext().getDevice();
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, layout, nullptr);
    }

    void bindDescriptorSets(VkCommandBuffer commandBuffer,
                            const std::vector<VkDescriptorSet> &descriptorSets,
                            const std::vector<uint32_t> &dynamicOffsets = {})
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0,
            static_cast<uint32_t>(descriptorSets.size()), descriptorSets.empty() ? nullptr : descriptorSets.data(),
            static_cast<uint32_t>(dynamicOffsets.size()), dynamicOffsets.empty() ? nullptr : dynamicOffsets.data());
    }

    void pushConstants(VkCommandBuffer commandBuffer,
                       VkShaderStageFlags flags,
                       uint32_t dataSize, const void *pData)
    {
        vkCmdPushConstants(commandBuffer, layout, flags, 0, dataSize, pData);
    }

private:
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
};