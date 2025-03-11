#pragma once
#include "compute_pipeline.hpp"
#include "radfoam.hpp"

class Renderer
{
public:
    struct alignas(16) UniformBuffer
    {
        glm::vec3 cameraPosition;
        uint32_t startPoint;
        glm::mat4 projectionMatrix;
        glm::mat4 viewMatrix;
        uint32_t width;
        uint32_t height;
        float tanFovx;
        float tanFovy;
    };

    Renderer(std::shared_ptr<RadFoam> pModel, std::shared_ptr<AABBTree> pAABB);

    void render();

private:
    std::shared_ptr<RadFoam> pModel;
    std::shared_ptr<AABBTree> pAABB;

    std::shared_ptr<Buffer> uniformBuffer;

    void handleInput();
    void updateUniform();
};