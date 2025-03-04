#include "RadFoam.hpp"
#include <stdexcept>

RadFoam::RadFoam(std::shared_ptr<RadFoamVulkanArgs> pArgs)
{
    try
    {
        std::cout << "Loading " << pArgs->scenePath << " to CPU." << std::endl;

        happly::PLYData ply(pArgs->scenePath);
        if (!ply.hasElement("vertex") || !ply.hasElement("adjacency"))
        {
            throw std::runtime_error("PLY file is not completed.");
        }

        auto &vert_elem = ply.getElement("vertex");
        const size_t vertex_count = vert_elem.count;

        auto positions = ply.getVertexPositions();
        auto colors = ply.getVertexColors();
        auto densities = vert_elem.getProperty<float>("density");
        auto offsets = vert_elem.getProperty<uint32_t>("adjacency_offset");

        std::array<std::vector<float>, 45> sh_vectors;
        for (int i = 0; i < 45; ++i)
        {
            std::string prop_name = "color_sh_" + std::to_string(i);
            sh_vectors[i] = vert_elem.getProperty<float>(prop_name);
        }

        vertices.resize(vertex_count);
        for (size_t i = 0; i < vertex_count; ++i)
        {
            auto &v = vertices[i];
            v.pos_density = {
                static_cast<float>(positions[i][0]),
                static_cast<float>(positions[i][1]),
                static_cast<float>(positions[i][2]),
                static_cast<float>(densities[i])};

            v.color = {
                static_cast<uint8_t>(colors[i][0]),
                static_cast<uint8_t>(colors[i][1]),
                static_cast<uint8_t>(colors[i][2]),
                0};

            v.adjacency_offset = offsets[i];

            for (int j = 0; j < 45; ++j)
            {
                v.sh_coeffs[j] = sh_vectors[j][i];
            }
        }

        auto &adj_elem = ply.getElement("adjacency");
        adjacency.indices = adj_elem.getProperty<uint32_t>("adjacency");
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(std::string("PLY read Error: ") + e.what());
    }
}

void RadFoam::loadRadFoam()
{
    std::cout << "Loading scene fronm CPU to GPU" << std::endl; 

    auto &context = VulkanContext::getContext();
    const size_t vertexBufferSize = sizeof(RadFoamVertex) * vertices.size();
    const size_t adjacencyBufferSize = sizeof(uint32_t) * adjacency.indices.size();

    // Create Vertex Buffer
    {
        // Staging buffer
        VkBufferCreateInfo bufferCI{};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = vertexBufferSize;
        bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocCI{};
        allocCI.usage = VMA_MEMORY_USAGE_AUTO;
        allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                        VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuffer;
        VmaAllocation stagingAlloc;
        VmaAllocationInfo allocInfo;
        ERR_GUARD_VULKAN(vmaCreateBuffer(context.getAllocator(), &bufferCI, &allocCI,
                                         &stagingBuffer, &stagingAlloc, &allocInfo));
        memcpy(allocInfo.pMappedData, vertices.data(), vertexBufferSize);

        // Device buffer
        bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        allocCI.flags = 0;
        allocCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        ERR_GUARD_VULKAN(vmaCreateBuffer(context.getAllocator(), &bufferCI, &allocCI,
                                         &buffer.vertexBuffer, &buffer.vertexBufferAlloc, nullptr));

        // Copy operation
        auto cmd = context.beginSingleTimeCommands();
        VkBufferCopy copyRegion{0, 0, vertexBufferSize};
        vkCmdCopyBuffer(cmd, stagingBuffer, buffer.vertexBuffer, 1, &copyRegion);
        context.endSingleTimeCommands(cmd);

        vmaDestroyBuffer(context.getAllocator(), stagingBuffer, stagingAlloc);
    }

    // Create Adjacency Buffer
    {
        // Staging buffer
        VkBufferCreateInfo bufferCI{};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = adjacencyBufferSize;
        bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocCI{};
        allocCI.usage = VMA_MEMORY_USAGE_AUTO;
        allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                        VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuffer;
        VmaAllocation stagingAlloc;
        VmaAllocationInfo allocInfo;
        ERR_GUARD_VULKAN(vmaCreateBuffer(context.getAllocator(), &bufferCI, &allocCI,
                                         &stagingBuffer, &stagingAlloc, &allocInfo));
        memcpy(allocInfo.pMappedData, adjacency.indices.data(), adjacencyBufferSize);

        // Device buffer
        bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        allocCI.flags = 0;
        allocCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        ERR_GUARD_VULKAN(vmaCreateBuffer(context.getAllocator(), &bufferCI, &allocCI,
                                         &buffer.adjacencyBuffer, &buffer.adjacencyBufferAlloc, nullptr));

        // Copy operation
        auto cmd = context.beginSingleTimeCommands();
        VkBufferCopy copyRegion{0, 0, adjacencyBufferSize};
        vkCmdCopyBuffer(cmd, stagingBuffer, buffer.adjacencyBuffer, 1, &copyRegion);
        context.endSingleTimeCommands(cmd);

        vmaDestroyBuffer(context.getAllocator(), stagingBuffer, stagingAlloc);
    }

    std::cout << "Loading Completed." << std::endl;
}

void RadFoam::unloadRadFoam()
{
    auto &context = VulkanContext::getContext();
    vmaDestroyBuffer(context.getAllocator(), buffer.vertexBuffer, buffer.vertexBufferAlloc);
    vmaDestroyBuffer(context.getAllocator(), buffer.adjacencyBuffer, buffer.adjacencyBufferAlloc);
}