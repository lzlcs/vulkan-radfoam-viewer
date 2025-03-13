#include "buffer.hpp"

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage,
               VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags memoryFlags,
               bool hostVisible)
    : size(size), hostVisible(hostVisible)
{
    VkBufferCreateInfo bufferCI{};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = size;
    bufferCI.usage = usage;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCI{};
    allocCI.usage = memoryUsage;
    allocCI.flags = memoryFlags;

    if (hostVisible)
    {
        allocCI.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        allocCI.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    auto allocator = VulkanContext::getContext().getAllocator();
    ERR_GUARD_VULKAN(vmaCreateBuffer(allocator, &bufferCI, &allocCI,
                                     &buffer, &allocation, nullptr));

    if (hostVisible)
    {
        vmaMapMemory(allocator, allocation, &mappedData);
    }
}

Buffer::~Buffer()
{
    if (buffer != VK_NULL_HANDLE)
    {
        auto allocator = VulkanContext::getContext().getAllocator();
        if (hostVisible && mappedData != nullptr)
        {
            vmaUnmapMemory(allocator, allocation);
        }
        vmaDestroyBuffer(VulkanContext::getContext().getAllocator(), buffer, allocation);
    }
}

void Buffer::uploadData(const void *data, VkDeviceSize dataSize, VkDeviceSize offset)
{
    assert(offset + dataSize <= size && "Data exceeds buffer size");
    if (hostVisible)
    {
        memcpy(static_cast<char *>(mappedData) + offset, data, dataSize);
        return;
    }

    Buffer stagingBuffer(size,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                         VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                             VMA_ALLOCATION_CREATE_MAPPED_BIT);

    auto &context = VulkanContext::getContext();
    auto allocator = context.getAllocator();

    void *mappedData;
    vmaMapMemory(allocator, stagingBuffer.allocation, &mappedData);
    memcpy(static_cast<char *>(mappedData) + offset, data, static_cast<size_t>(dataSize));
    vmaUnmapMemory(allocator, stagingBuffer.allocation);

    auto cmd = context.beginSingleTimeCommands();
    VkBufferCopy copyRegion{0, 0, size};
    vkCmdCopyBuffer(cmd, stagingBuffer.buffer, buffer, 1, &copyRegion);
    context.endSingleTimeCommands(cmd);
}

void Buffer::downloadData(void *data, VkDeviceSize dataSize, VkDeviceSize offset)
{
    assert(dataSize <= size && "Data size exceeds buffer capacity");

    if (hostVisible)
    {
        memcpy(data, static_cast<char *>(mappedData) + offset, dataSize);
        return;
    }

    Buffer stagingBuffer(dataSize,
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                         VMA_MEMORY_USAGE_CPU_ONLY,
                         VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

    auto &context = VulkanContext::getContext();
    auto allocator = context.getAllocator();

    auto cmd = context.beginSingleTimeCommands();
    VkBufferCopy copyRegion{0, 0, size};
    vkCmdCopyBuffer(cmd, buffer, stagingBuffer.buffer, 1, &copyRegion);
    context.endSingleTimeCommands(cmd);

    void *mappedData;
    vmaMapMemory(allocator, stagingBuffer.allocation, &mappedData);
    memcpy(data, static_cast<char *>(mappedData) + offset, dataSize);
    vmaUnmapMemory(allocator, stagingBuffer.allocation);
}

Image::Image(VkImageType type, VkFormat format, VkExtent3D extent,
             VkImageUsageFlags usage, VmaMemoryUsage memoryUsage,
             VkImageTiling tiling, uint32_t mipLevels, uint32_t arrayLayers,
             VkSampleCountFlagBits samples)
    : format(format), extent(extent), mipLevels(mipLevels)
{

    VkImageCreateInfo imageCI{};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType = type;
    imageCI.format = format;
    imageCI.extent = extent;
    imageCI.mipLevels = mipLevels;
    imageCI.arrayLayers = arrayLayers;
    imageCI.samples = samples;
    imageCI.tiling = tiling;
    imageCI.usage = usage;
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocCI{};
    allocCI.usage = memoryUsage;
    auto allocator = VulkanContext::getContext().getAllocator();
    ERR_GUARD_VULKAN(vmaCreateImage(allocator, &imageCI, &allocCI,
                                    &image, &allocation, nullptr));
}

Image::~Image()
{
    if (image != VK_NULL_HANDLE)
    {
        auto allocator = VulkanContext::getContext().getAllocator();
        vmaDestroyImage(allocator, image, allocation);
    }
}

// void Image::uploadData(const void *data, VkDeviceSize dataSize, VkImageLayout finalLayout)
// {
//     Buffer stagingBuffer(dataSize,
//                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//                          VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
//                          VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

//     stagingBuffer.uploadData(data, dataSize);

//     auto &context = VulkanContext::getContext();
//     auto cmd = context.beginSingleTimeCommands();

//     // 转换布局为 TRANSFER_DST_OPTIMAL
//     transitionLayout(cmd, currentLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
//                      VK_PIPELINE_STAGE_TRANSFER_BIT);

//     // 执行缓冲区到图像的拷贝
//     VkBufferImageCopy copyRegion{};
//     copyRegion.bufferOffset = 0;
//     copyRegion.bufferRowLength = 0;
//     copyRegion.bufferImageHeight = 0;
//     copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//     copyRegion.imageSubresource.mipLevel = 0;
//     copyRegion.imageSubresource.baseArrayLayer = 0;
//     copyRegion.imageSubresource.layerCount = 1;
//     copyRegion.imageOffset = {0, 0, 0};
//     copyRegion.imageExtent = extent;

//     vkCmdCopyBufferToImage(cmd, stagingBuffer.getBuffer(), image,
//                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

//     // 转换到最终布局（如 SHADER_READ）
//     transitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, finalLayout,
//                      VK_PIPELINE_STAGE_TRANSFER_BIT,
//                      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

//     context.endSingleTimeCommands(cmd);
// }

// void Image::transitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout,
//                              VkPipelineStageFlags srcStageMask,
//                              VkPipelineStageFlags dstStageMask)
// {
//     auto cmd = VulkanContext::getContext().beginSingleTimeCommands();
//     transitionLayout(cmd, oldLayout, newLayout, srcStageMask, dstStageMask);
//     VulkanContext::getContext().endSingleTimeCommands(cmd);
// }

// // 内部实现布局转换（需传入命令缓冲区）
// void Image::transitionLayout(VkCommandBuffer cmd, VkImageLayout oldLayout, VkImageLayout newLayout,
//                              VkPipelineStageFlags srcStageMask,
//                              VkPipelineStageFlags dstStageMask)
// {
//     VkImageMemoryBarrier barrier{};
//     barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//     barrier.oldLayout = oldLayout;
//     barrier.newLayout = newLayout;
//     barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//     barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//     barrier.image = image;
//     barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//     barrier.subresourceRange.baseMipLevel = 0;
//     barrier.subresourceRange.levelCount = mipLevels;
//     barrier.subresourceRange.baseArrayLayer = 0;
//     barrier.subresourceRange.layerCount = 1;

//     // 根据布局设置访问掩码
//     switch (oldLayout)
//     {
//     case VK_IMAGE_LAYOUT_UNDEFINED:
//         barrier.srcAccessMask = 0;
//         break;
//     case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
//         barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//         break;
//     case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
//         barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
//         break;
//         // 其他布局处理...
//     }

//     switch (newLayout)
//     {
//     case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
//         barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//         break;
//     case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
//         barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//         break;
//         // 其他布局处理...
//     }

//     vkCmdPipelineBarrier(cmd, srcStageMask, dstStageMask, 0,
//                          0, nullptr, 0, nullptr, 1, &barrier);

//     currentLayout = newLayout;
// }
