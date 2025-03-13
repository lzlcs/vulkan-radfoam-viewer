#include "renderer.hpp"
#include "radfoam.hpp"

Renderer::Renderer(std::shared_ptr<RadFoamVulkanArgs> pArgs,
                   std::shared_ptr<RadFoam> pModel,
                   std::shared_ptr<AABBTree> pAABB)
    : pArgs(pArgs), pModel(pModel), pAABB(pAABB)
{
    auto &context = VulkanContext::getContext();
    uniformBuffer = std::make_shared<Buffer>(sizeof(UniformData),
                                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                             VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                                             0, true);

    // VkCommandBufferAllocateInfo allocInfo{};
    // allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // allocInfo.commandPool = context.getCommandPool();
    // allocInfo.commandBufferCount = 1;
    // ERR_GUARD_VULKAN(vkAllocateCommandBuffers(context.getDevice(), &allocInfo, &renderCommandBuffer));

    // VkFenceCreateInfo fenceInfo{};
    // fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // ERR_GUARD_VULKAN(vkCreateFence(context.getDevice(), &fenceInfo, nullptr, &inFlightFence));

    createRayTracingPipeline();
}

Renderer::~Renderer()
{
    auto &context = VulkanContext::getContext();

    if (renderCommandBuffer != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(context.getDevice(), context.getCommandPool(), 1, &renderCommandBuffer);
    }

    if (inFlightFence != VK_NULL_HANDLE)
    {
        vkDestroyFence(context.getDevice(), inFlightFence, nullptr);
    }
}

void Renderer::render()
{
    handleInput();
    updateUniform();
    recordRenderCommandBuffer();
    // submitRenderCommandBuffer();

    // std::vector<Ray> tmp(rayBuffer->getSize() / sizeof(Ray));
    // rayBuffer->downloadData(tmp.data(), rayBuffer->getSize());

    // int x = 405600 - 5;
    // for (int i = x; i < x + 5; i++)
    // {
    //     std::cout << tmp[i].origin[0] << ' ' << tmp[i].origin[1] << ' ' << tmp[i].origin[2] << std::endl;

    //     std::cout << tmp[i].direction[0] << ' ' << tmp[i].direction[1] << ' ' << tmp[i].direction[2] << std::endl;
    // }

    std::vector<glm::vec4> tmp(rgbBuffer->getSize() / sizeof(glm::vec4));
    rgbBuffer->downloadData(tmp.data(), rgbBuffer->getSize());

    int pix = 0;
    std::cout << tmp.size() << ' ' << pix << std::endl;
    std::cout << tmp[pix][0] << ' ';
    std::cout << tmp[pix][1] << ' ';
    std::cout << tmp[pix][2] << ' ';
    std::cout << tmp[pix][3] << std::endl;

}

void Renderer::handleInput()
{
}

void Renderer::updateUniform()
{
    UniformData data;

    data.R = glm::mat3x4(
        {0.08240976184606552, 0.6311448812484741, -0.771274745464325, 0},
        {-0.6168570518493652, 0.6401463150978088, 0.45793023705482483, 0},
        {0.7827489972114563, 0.43802836537361145, 0.4420804977416992, 0});
    data.T = glm::vec3(-3.1629207134246826, -0.6483269333839417, -0.17025022208690643);

    data.startPoint = pAABB->nearestNeighbor(data.T);
    data.focal_x = 805.67529296875;
    data.focal_y = 805.67529296875;
    data.width = pArgs->windowWidth;
    data.height = pArgs->windowHeight;
    data.maxSteps = 1024;
    data.transmittanceThreshold = 0.001f;

    // std::cout << "start point: " << data.startPoint << std::endl;

    uniformBuffer->uploadData(&data, sizeof(UniformData));

    // UniformData tmp{};
    // uniformBuffer->downloadData(&tmp, sizeof(UniformData));
    // std::cout << tmp.T[0] << std::endl;
    // std::cout << tmp.T[1] << std::endl;
    // std::cout << tmp.T[2] << std::endl;
    // std::cout << tmp.startPoint << std::endl;
}

void Renderer::createRayTracingPipeline()
{
    auto &context = VulkanContext::getContext();
    auto shader = std::make_shared<Shader>("src/shader/spv/ray_tracing.comp.spv");
    auto rgbBufferSize = pArgs->windowHeight * pArgs->windowWidth * sizeof(glm::vec4);

    rgbBuffer = std::make_shared<Buffer>(rgbBufferSize,
                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                         VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
    // Input Bindings
    std::vector<DescriptorSet::BindingInfo> inputBindings = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
        {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT},
    };
    auto inputSet = std::make_shared<DescriptorSet>(inputBindings);
    inputSet->bindBuffers(0, {uniformBuffer->getBuffer()});
    inputSet->bindBuffers(1, {pModel->getVertexBuffer()->getBuffer()});
    inputSet->bindBuffers(2, {pModel->getAdjacencyBuffer()->getBuffer()});

    // Output Bindings
    std::vector<DescriptorSet::BindingInfo> outputBindings = {
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT}};
    auto outputSet = std::make_shared<DescriptorSet>(outputBindings);
    outputSet->bindBuffers(0, {rgbBuffer->getBuffer()});

    // Create Pipeline
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{
        inputSet->getDescriptorSetLayout(), outputSet->getDescriptorSetLayout()};
    rayTracingPipeline = std::make_shared<ComputePipeline>(shader->shaderModule, descriptorSetLayouts);
    rayTracingPipeline->addDescriptorSet(inputSet);
    rayTracingPipeline->addDescriptorSet(outputSet);
}

void Renderer::recordRenderCommandBuffer()
{
    auto &context = VulkanContext::getContext();

    auto cmd = context.beginSingleTimeCommands();
    auto numGroups = (pArgs->windowHeight * pArgs->windowWidth + 255) / 256;
    rayTracingPipeline->bindDescriptorSets(cmd);
    vkCmdDispatch(cmd, numGroups, 1, 1);
    context.endSingleTimeCommands(cmd);


    // vkResetCommandBuffer(renderCommandBuffer, 0);
    // VkCommandBufferBeginInfo beginInfo{};
    // beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // vkBeginCommandBuffer(renderCommandBuffer, &beginInfo);

    // auto numGroups = (pArgs->windowHeight * pArgs->windowWidth + 255) / 256;
    // rayTracingPipeline->bindDescriptorSets(renderCommandBuffer);
    // vkCmdDispatch(renderCommandBuffer, numGroups, 1, 1);

    // VkMemoryBarrier barrier = {
    //     .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
    //     .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
    //     .dstAccessMask = VK_ACCESS_SHADER_READ_BIT};
    // vkCmdPipelineBarrier(renderCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                      0, 1, &barrier, 0, nullptr, 0, nullptr);

    // vkEndCommandBuffer(renderCommandBuffer);
}

void Renderer::submitRenderCommandBuffer()
{
    auto &context = VulkanContext::getContext();
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &renderCommandBuffer;
    vkResetFences(context.getDevice(), 1, &inFlightFence);

    ERR_GUARD_VULKAN(vkQueueSubmit(context.getQueue("compute"), 1, &submitInfo, inFlightFence));

    vkWaitForFences(context.getDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);
}