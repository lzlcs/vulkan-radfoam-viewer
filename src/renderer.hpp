#pragma once
#include "compute_pipeline.hpp"
#include "radfoam.hpp"

class GLFWwindow;

class Renderer
{
public:
    struct UniformData
    {
        alignas(16) glm::mat3x4 R; // Just for memory alignment
        alignas(16) glm::vec3 T;
        uint32_t width;
        uint32_t height;
        float focal_x;
        float focal_y;
        uint32_t startPoint;
        uint32_t maxSteps;
        float transmittanceThreshold;
    };

    static_assert(sizeof(UniformData) == 24 * sizeof(int));
    static_assert(offsetof(UniformData, T) == 12 * sizeof(int));
    static_assert(offsetof(UniformData, width) == 15 * sizeof(int));

    Renderer(std::shared_ptr<RadFoamVulkanArgs> pArgs,
             std::shared_ptr<RadFoam> pModel,
             std::shared_ptr<AABBTree> pAABB);
    ~Renderer();

    void render();

private:
    float moveSpeed = 0.05f;
    float rotateSpeed = glm::radians(1.0f);

    std::shared_ptr<RadFoamVulkanArgs> pArgs;
    std::shared_ptr<RadFoam> pModel;
    std::shared_ptr<AABBTree> pAABB;

    VkCommandBuffer renderCommandBuffer = VK_NULL_HANDLE;

    std::shared_ptr<Buffer> uniformBuffer;

    std::shared_ptr<ComputePipeline> rayTracingPipeline;

    VkFence inFlightFence = VK_NULL_HANDLE;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    UniformData data;

    void handleInput();
    void updateUniform();
    void createRayTracingPipeline();
    void createSyncObjects();
    void recordRenderCommandBuffer(uint32_t imageIndex);
    void submitRenderCommandBuffer();
};