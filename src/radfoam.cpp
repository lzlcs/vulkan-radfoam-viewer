#include "RadFoam.hpp"
#include <stdexcept>
#include <fstream>

#pragma pack(push, 1)
struct RawVertex
{
    float x, y, z;
    uint8_t r, g, b;
    float density;
    uint32_t adjacency_offset;
    float sh_coeffs[45];
};

#pragma pack(pop)

void RadFoam::parseHeader(std::istream &is)
{
    std::string line;

    std::getline(is, line);
    assert(line == "ply" && "Invalid PLY file");

    std::getline(is, line);
    assert(line.find("binary_little_endian") != std::string::npos &&
           "Only binary little-endian format supported");

    std::getline(is, line);
    int vertexScanResult = sscanf(line.c_str(), "element vertex %u", &numVertices);
    assert(vertexScanResult == 1 && "Missing vertex element");

    for (int i = 0; i < 53; ++i)
        assert(std::getline(is, line) && "Unexpected end of header");

    std::getline(is, line);
    int adjScanResult = sscanf(line.c_str(), "element adjacency %u", &numAdjacency);
    assert(adjScanResult == 1 && "Missing adjacency element");

    std::getline(is, line);
    assert(line == "property uint adjacency" && "Invalid adjacency property");

    std::getline(is, line);
    assert(line == "end_header" && "Header not properly terminated");
}

void RadFoam::parseVertexData(std::istream &is)
{
    vertices.resize(numVertices);

    std::vector<RawVertex> rawVertices(numVertices);
    is.read(reinterpret_cast<char *>(rawVertices.data()),
            numVertices * sizeof(RawVertex));

    assert((is.gcount() == numVertices * sizeof(RawVertex)) &&
           "Vertex data incomplete");

    for (size_t i = 0; i < numVertices; ++i)
    {
        auto &src = rawVertices[i];
        auto &dst = vertices[i];
        dst.pos_offset = glm::vec4(src.x, src.y, src.z, src.adjacency_offset);
        dst.color = glm::u8vec4(src.r, src.g, src.b, 255);
        dst.density = src.density;
        std::copy_n(src.sh_coeffs, 45, dst.sh_coeffs.begin());
    }
}

void RadFoam::parseAdjacencyData(std::istream &is)
{
    adjacency.resize(numAdjacency);
    is.read(reinterpret_cast<char *>(adjacency.data()),
            numAdjacency * sizeof(uint32_t));

    assert((is.gcount() == numAdjacency * sizeof(uint32_t)) && "Adjacency data incomplete");
}

RadFoam::RadFoam(std::shared_ptr<RadFoamVulkanArgs> pArgs)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    std::ifstream ifs(pArgs->scenePath, std::ios::binary);
    assert(ifs && ("Failed to open PLY file."));

    parseHeader(ifs);
    parseVertexData(ifs);
    parseAdjacencyData(ifs);
    uploadRadFoam();

    auto endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Loading RadFoam Scene: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count()
              << "ms" << std::endl;
}

void RadFoam::uploadRadFoam()
{
    auto &context = VulkanContext::getContext();
    const size_t vertexBufferSize = sizeof(RadFoamVertex) * vertices.size();
    const size_t adjacencyBufferSize = sizeof(uint32_t) * adjacency.size();

    vertexBuffer = std::make_shared<Buffer>(vertexBufferSize,
                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    vertexBuffer->uploadData(vertices.data(), vertexBufferSize);

    adjacencyBuffer = std::make_shared<Buffer>(adjacencyBufferSize,
                                               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                               VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
    adjacencyBuffer->uploadData(adjacency.data(), adjacencyBufferSize);
}

AABBTree::AABBTree(std::shared_ptr<RadFoam> pModel) : pModel(pModel)
{
    auto getLevel = [](uint32_t x)
    { return x > 1 ? glm::log2(x - 1) + 1 : 1; };

    auto numVertices = pModel->getNumVertices();
    numLevels = getLevel(numVertices);

    size_t aabbBufferSize = sizeof(AABB) * (1 << numLevels);
    aabbBuffer = std::make_shared<Buffer>(aabbBufferSize,
                                          VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                          VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
    std::cout << "Initialize AABB Tree: numLevels(" << numLevels << ")" << std::endl;
}

void AABBTree::buildAABBLeaves()
{
    auto &context = VulkanContext::getContext();
    auto shader = std::make_shared<Shader>("src/shader/spv/build_leaves.comp.spv");

    // Create descriptor set
    std::vector<DescriptorSet::BindingInfo> bindings = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT}, // Vertex Buffer
        {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT}  // AABB Buffer
    };
    auto set = std::make_shared<DescriptorSet>(bindings);
    set->bindBuffers(0, {pModel->getVertexBuffer()->getBuffer()});
    set->bindBuffers(1, {aabbBuffer->getBuffer()});

    // Create compute pipeline
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{set->getDescriptorSetLayout()};
    std::vector<VkDescriptorSet> descriptorSets{set->getDescriptorSet()};
    std::vector<VkPushConstantRange> pushConstants;
    auto pipeline = std::make_shared<ComputePipeline>(
        shader->shaderModule, descriptorSetLayouts, pushConstants);

    auto cmd = context.beginSingleTimeCommands();
    pipeline->bindDescriptorSets(cmd, descriptorSets);
    auto workGroups = ((1 << numLevels - 1) + 255) / 256;
    vkCmdDispatch(cmd, workGroups, 1, 1);

    VkMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT};
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0, 1, &barrier, 0, nullptr, 0, nullptr);

    context.endSingleTimeCommands(cmd);

    // std::vector<AABB> tmp(1u << numLevels);
    // aabbBuffer->downloadData(tmp.data(), aabbBuffer->getSize());

    // int x = 983388;
    // std::cout << x << std::endl;
    // for (int i = x - 5; i < x; i++)
    // {
    //     std::cout << tmp[i].min[0] << ' ';
    //     std::cout << tmp[i].min[1] << ' ';
    //     std::cout << tmp[i].min[2] << std::endl;
    //     std::cout << tmp[i].max[0] << ' ';
    //     std::cout << tmp[i].max[1] << ' ';
    //     std::cout << tmp[i].max[2] << std::endl;
    //     // std::cout << pModel->vertices[i * 2].pos_offset[0] << ' ';
    //     // std::cout << pModel->vertices[i * 2].pos_offset[1] << ' ';
    //     // std::cout << pModel->vertices[i * 2].pos_offset[2] << std::endl;
    //     // std::cout << pModel->vertices[i * 2 + 1].pos_offset[0] << ' ';
    //     // std::cout << pModel->vertices[i * 2 + 1].pos_offset[1] << ' ';
    //     // std::cout << pModel->vertices[i * 2 + 1].pos_offset[2] << std::endl;
    //     std::cout << std::endl;
    // }
}

void AABBTree::buildAABBTree()
{
    if (numLevels == 1)
        return;

    auto &context = VulkanContext::getContext();
    auto shader = std::make_shared<Shader>("src/shader/spv/build_tree.comp.spv");

    // Create descriptor set
    std::vector<DescriptorSet::BindingInfo> bindings = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT}, // AABB Buffer
    };
    auto set = std::make_shared<DescriptorSet>(bindings);
    set->bindBuffers(0, {aabbBuffer->getBuffer()});

    // Create compute pipeline
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{set->getDescriptorSetLayout()};
    std::vector<VkDescriptorSet> descriptorSets{set->getDescriptorSet()};
    std::vector<VkPushConstantRange> pushConstants{
        {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(Constants)}};
    auto pipeline = std::make_shared<ComputePipeline>(
        shader->shaderModule, descriptorSetLayouts, pushConstants);

    auto cmd = context.beginSingleTimeCommands();
    pipeline->bindDescriptorSets(cmd, descriptorSets);
    for (int i = numLevels - 2; i >= 0; i--)
    {
        Constants cons{(1u << i),                          // Total nodes in this level
                       (1u << numLevels) - (1u << i + 1)}; // First node's offset in AABB Buffer

        pipeline->pushConstants(cmd, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(Constants), &cons);

        auto workGroups = (cons.numNodes + 255) / 256;
        vkCmdDispatch(cmd, workGroups, 1, 1);

        VkMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT};
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0, 1, &barrier, 0, nullptr, 0, nullptr);
    }
    context.endSingleTimeCommands(cmd);

    // std::vector<AABB> tmp(1u << numLevels);
    // aabbBuffer->downloadData(tmp.data(), aabbBuffer->getSize());

    // int x = 2097152;
    // // int x = 1048576 + 3;
    // std::cout << x << std::endl;
    // for (int i = x - 5; i < x; i++)
    // {
    //     std::cout << tmp[i].min[0] << ' ';
    //     std::cout << tmp[i].min[1] << ' ';
    //     std::cout << tmp[i].min[2] << std::endl;
    //     std::cout << tmp[i].max[0] << ' ';
    //     std::cout << tmp[i].max[1] << ' ';
    //     std::cout << tmp[i].max[2] << std::endl;
    //     // std::cout << pModel->vertices[i * 2].pos_offset[0] << ' ';
    //     // std::cout << pModel->vertices[i * 2].pos_offset[1] << ' ';
    //     // std::cout << pModel->vertices[i * 2].pos_offset[2] << std::endl;
    //     // std::cout << pModel->vertices[i * 2 + 1].pos_offset[0] << ' ';
    //     // std::cout << pModel->vertices[i * 2 + 1].pos_offset[1] << ' ';
    //     // std::cout << pModel->vertices[i * 2 + 1].pos_offset[2] << std::endl;
    //     std::cout << std::endl;
    // }
}