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
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = context.getCommandPool();
    allocInfo.commandBufferCount = 1;
    ERR_GUARD_VULKAN(vkAllocateCommandBuffers(context.getDevice(), &allocInfo, &renderCommandBuffer));

    createRayTracingPipeline();
    createSyncObjects();
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

    if (renderFinishedSemaphore != VK_NULL_HANDLE)
        vkDestroySemaphore(context.getDevice(), renderFinishedSemaphore, nullptr);
    if (imageAvailableSemaphore != VK_NULL_HANDLE)
        vkDestroySemaphore(context.getDevice(), imageAvailableSemaphore, nullptr);
}

void Renderer::render()
{
    auto &context = VulkanContext::getContext();
    vkWaitForFences(context.getDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(context.getDevice(), 1, &inFlightFence);

    handleInput();
    updateUniform();

    uint32_t imageIndex;
    vkAcquireNextImageKHR(context.getDevice(), context.getSwapChain(), UINT64_MAX,
                          imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(renderCommandBuffer, 0);
    recordRenderCommandBuffer(imageIndex);

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &renderCommandBuffer;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    ERR_GUARD_VULKAN(vkQueueSubmit(context.getQueue("compute"), 1, &submitInfo, inFlightFence));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {context.getSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(context.getQueue("present"), &presentInfo);
    // submitRenderCommandBuffer();

    // std::vector<Ray> tmp(rayBuffer->getSize() / sizeof(Ray));
    // rayBuffer->downloadData(tmp.data(), rayBuffer->getSize());

    // int x = 405600 - 5;
    // for (int i = x; i < x + 5; i++)
    // {
    //     std::cout << tmp[i].origin[0] << ' ' << tmp[i].origin[1] << ' ' << tmp[i].origin[2] << std::endl;

    //     std::cout << tmp[i].direction[0] << ' ' << tmp[i].direction[1] << ' ' << tmp[i].direction[2] << std::endl;
    // }

    // int pix = 623 * pArgs->windowWidth + 131;
    // int pix = 177 * pArgs->windowWidth + 454;

    // std::vector<uint8_t> tmp(rgbBuffer->getSize() * 4);
    // rgbBuffer->downloadData(tmp.data(), rgbBuffer->getSize());

    // std::cout << tmp.size() << ' ' << pix << std::endl;
    // std::cout << (int)tmp[pix * 4] << ' ';
    // std::cout << (int)tmp[pix * 4 + 1] << ' ';
    // std::cout << (int)tmp[pix * 4 + 2] << ' ';
    // std::cout << (int)tmp[pix * 4 + 3] << std::endl;

    // std::vector<uint32_t> tmp(rgbBuffer->getSize());
    // rgbBuffer->downloadData(tmp.data(), rgbBuffer->getSize());

    // std::cout << tmp[pix] << std::endl;
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
    auto rgbBufferSize = pArgs->windowHeight * pArgs->windowWidth * sizeof(int);

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

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{inputSet->getDescriptorSetLayout()};
    std::vector<std::shared_ptr<DescriptorSet>> outputSets;

    // Output Bindings for Every Swapchain Image
    auto imageCounts = context.getSwapChainImageCount();
    for (int i = 0; i < imageCounts; i++)
    {
        std::vector<DescriptorSet::BindingInfo> outputBindings = {
            {0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT}};
        auto outputSet = std::make_shared<DescriptorSet>(outputBindings);
        // outputSet->bindBuffers(0, {rgbBuffer->getBuffer()});
        outputSet->bindImages(0, {context.getSwapChainImageView(i)}, VK_IMAGE_LAYOUT_GENERAL);

        outputSets.push_back(outputSet);
        descriptorSetLayouts.push_back(outputSet->getDescriptorSetLayout());
    }

    // Create Pipeline
    rayTracingPipeline = std::make_shared<ComputePipeline>(shader->shaderModule, descriptorSetLayouts);
    rayTracingPipeline->addDescriptorSet(inputSet);
    for (int i = 0; i < imageCounts; i++)
    {
        rayTracingPipeline->addDescriptorSet(outputSets[i]);
    }
}

void Renderer::createSyncObjects()
{
    auto device = VulkanContext::getContext().getDevice();
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    ERR_GUARD_VULKAN(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore));
    ERR_GUARD_VULKAN(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore));
    ERR_GUARD_VULKAN(vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence));
}

void Renderer::recordRenderCommandBuffer(uint32_t imageIndex)
{
    auto &context = VulkanContext::getContext();

    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(renderCommandBuffer, &beginInfo);
    
    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .image = context.getSwapChainImage(imageIndex),
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    vkCmdPipelineBarrier(
        renderCommandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);


    auto numGroups = (pArgs->windowHeight * pArgs->windowWidth + 255) / 256;
    // std::cout << imageIndex << std::endl;
    rayTracingPipeline->bindDescriptorSets(renderCommandBuffer, {0, imageIndex + 1});
    vkCmdDispatch(renderCommandBuffer, numGroups, 1, 1);


    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

    vkCmdPipelineBarrier(
        renderCommandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(renderCommandBuffer);
}
