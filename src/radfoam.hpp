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
    auto &getVertices() { return vertices; }

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

    struct Constants
    {
        uint32_t numNodes;
        uint32_t offset;
    };

    AABBTree(std::shared_ptr<RadFoam> pModel);
    uint32_t nearestNeighbor(glm::vec3 &pos);

private:
    void buildAABBLeaves();
    void buildAABBTree();
    void downloadAABBTree();
    std::shared_ptr<RadFoam> pModel;
    std::shared_ptr<Buffer> aabbBuffer;
    std::vector<AABB> aabbTree;
    uint32_t numLevels;
};