#include "RadFoam.hpp"
#include <stdexcept>
#include <miniply/miniply.h>

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
        dst.pos_density = glm::vec4(src.x, src.y, src.z, src.density);
        dst.color = glm::u8vec4(src.r, src.g, src.b, 255);
        dst.adjacency_offset = src.adjacency_offset;
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
                                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    vertexBuffer->uploadData(vertices.data(), vertexBufferSize);

    adjacencyBuffer = std::make_shared<Buffer>(adjacencyBufferSize,
                                               VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                               VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
    adjacencyBuffer->uploadData(adjacency.data(), adjacencyBufferSize);
}