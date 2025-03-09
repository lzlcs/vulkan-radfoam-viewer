#pragma once

#include "arguments.hpp"
#include <vulkan/vulkan.h>
#include "vulkan_context.h"
#include "compute_pipeline.hpp"
#include <glm/glm.hpp>
#include <array>
#include <memory>
#include <vector>
#include "buffer.hpp"

class RadFoam
{
public:
    struct RadFoamVertex
    {
        alignas(16) glm::vec4 pos_offset;
        alignas(16) glm::u8vec4 color;
        float density;
        alignas(16) std::array<float, 45> sh_coeffs;
    };

    static_assert(sizeof(RadFoamVertex) == 56 * sizeof(float), "RadFoamVertex size mismatch");

    explicit RadFoam(std::shared_ptr<RadFoamVulkanArgs> pArgs);

    auto getNumVertices() { return this->numVertices; }
    auto getNumAdjacency() { return this->numAdjacency; }
    auto getVertexBuffer() { return vertexBuffer; }
    auto getAdjacencyBuffer() { return adjacencyBuffer; }

// private:
    std::vector<RadFoamVertex> vertices;
    std::vector<uint32_t> adjacency;
    std::shared_ptr<Buffer> vertexBuffer;
    std::shared_ptr<Buffer> adjacencyBuffer;
    uint32_t numVertices;
    uint32_t numAdjacency;

    void parseHeader(std::istream &is);
    void parseVertexData(std::istream &is);
    void parseAdjacencyData(std::istream &is);
    void uploadRadFoam();
};

class AABBTree
{
public:
    struct AABB
    {
        alignas(16) glm::vec3 min;
        alignas(16) glm::vec3 max;
    };

    AABBTree(std::shared_ptr<RadFoam> pModel);
    void buildAABBLeaves();
    void buildAABBTree();

    // void buildAABBTree()
    // {

    //     auto shader = std::make_shared<Shader>("tree_builder.comp.spv");

    //     // Create descriptor set
    //     std::vector<DescriptorSet::BindingInfo> bindings = {
    //         {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT}, // Vertex Buffer
    //         {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT}  // AABB Buffer
    //     };
    //     auto set = std::make_shared<DescriptorSet>(bindings);
    //     set->bindBuffers(0, {pModel->getVertexBuffer().getBuffer()});
    //     set->bindBuffers(1, {aabbBuffer.getBuffer()});

    //     // Create compute pipeline
    //     std::vector<VkDescriptorSetLayout> descriptorSetLayouts{set->getDescriptorSetLayout()};
    //     std::vector<VkDescriptorSet> descriptorSets{set->getDescriptorSet()};
    //     std::vector<VkPushConstantRange> pushConstants{
    //         {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int)} // Currrent level
    //     };
    //     auto pipeline = std::make_shared<ComputePipeline>(
    //         shader->shaderModule, descriptorSetLayouts, pushConstants);

    //     // Build AABB Tree
    //     auto cmd = context.beginSingleTimeCommands();
    //     for (int i = numLevels - 1; i >= 0; i--)
    //     {
    //         pipeline->bindDescriptorSets(cmd, descriptorSets);
    //         pipeline->pushConstants(cmd, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(int), &i);
    //         auto workGroups = (glm::min((1u << i), numVertices) + 255) / 256;
    //         vkCmdDispatch(cmd, workGroups, 1, 1);

    //         VkMemoryBarrier barrier{
    //             .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
    //             .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
    //             .dstAccessMask = VK_ACCESS_SHADER_READ_BIT};
    //         vkCmdPipelineBarrier(
    //             cmd,
    //             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //             0, 1, &barrier, 0, nullptr, 0, nullptr);
    //     }
    //     context.endSingleTimeCommands(cmd);
    // }

private:
    std::shared_ptr<RadFoam> pModel;
    std::shared_ptr<Buffer> aabbBuffer;
    uint32_t numLevels;
};