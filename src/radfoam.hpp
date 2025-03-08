#pragma once

#include "arguments.hpp"
#include <vulkan/vulkan.h>
#include "vulkan_context.h"
#include "compute_pipeline.hpp"
#include <happly/happly.h>
#include <glm/glm.hpp>
#include <array>
#include <memory>
#include <vector>

class RadFoam
{
public:
    struct RadFoamVertex
    {
        alignas(16) glm::vec4 pos_density;
        alignas(16) glm::u8vec4 color;
        uint32_t adjacency_offset;
        alignas(16) std::array<float, 45> sh_coeffs;
    };

    struct RadFoamAdjacency
    {
        std::vector<uint32_t> indices;
    };
    static_assert(sizeof(RadFoamVertex) == 56 * sizeof(float), "RadFoamVertex size mismatch");

    struct RadFoamBuffers
    {
        VkBuffer vertexBuffer;
        VmaAllocation vertexBufferAlloc;
        VkBuffer adjacencyBuffer;
        VmaAllocation adjacencyBufferAlloc;
    };

    explicit RadFoam(std::shared_ptr<RadFoamVulkanArgs> pArgs);
    void loadRadFoam();
    void unloadRadFoam();

    auto getNumVertice() { return this->numVertice; }
    auto getNumAdjacency() { return this->numAdjacency; }

private:
    std::vector<RadFoamVertex> vertices;
    RadFoamAdjacency adjacency;
    RadFoamBuffers buffer{};
    uint32_t numVertice;
    uint32_t numAdjacency;
};

class AABBTree
{
public:
    struct AABB
    {
        alignas(16) glm::vec3 min;
        alignas(16) glm::vec3 max;
    };

    struct AABBBuffers
    {
        VkBuffer aabbTree;
        VkDeviceMemory treeMemory;

        uint32_t maxDepth;
        uint32_t numPoints;
    };

    void buildAABBTree()
    {
        auto &context = VulkanContext::getContext();
        auto getLevel = [](uint32_t x)
        { return x > 1 ? glm::log2(x - 1) + 1 : 1; };
        numLevels = getLevel(pModel->getNumVertice());

        auto shader = std::make_shared<Shader>("tree_builder.comp.spv");

        std::vector<DescriptorSet::BindingInfo> bindings = {
            {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT}, // Vertex Buffer
            {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT} // AABB Buffer
        };
        auto set = std::make_shared<DescriptorSet>(bindings);

    }

private:
    std::shared_ptr<RadFoam> pModel;
    AABBBuffers buffer;
    uint32_t numLevels;
};