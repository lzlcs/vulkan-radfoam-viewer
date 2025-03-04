#pragma once

#include "arguments.hpp"
#include <vulkan/vulkan.h>
#include "vulkan_context.h"
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

    struct RadFoamBuffers
    {
        VkBuffer vertexBuffer;
        VmaAllocation vertexBufferAlloc;
        VkBuffer adjacencyBuffer;
        VmaAllocation adjacencyBufferAlloc;
    };

    static_assert(sizeof(RadFoamVertex) == 56 * sizeof(float), "RadFoamVertex size mismatch");

    explicit RadFoam(std::shared_ptr<RadFoamVulkanArgs> pArgs);
    void loadRadFoam();
    void unloadRadFoam();

private:
    std::vector<RadFoamVertex> vertices;
    RadFoamAdjacency adjacency;
    RadFoamBuffers buffer{};
};