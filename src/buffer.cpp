#include "buffer.hpp"

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage,
               VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags memoryFlags)
    : size(size)
{
    VkBufferCreateInfo bufferCI{};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = size;
    bufferCI.usage = usage;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCI{};
    allocCI.usage = memoryUsage;
    allocCI.flags = memoryFlags;
    auto &context = VulkanContext::getContext();

    ERR_GUARD_VULKAN(vmaCreateBuffer(context.getAllocator(), &bufferCI, &allocCI,
                                     &buffer, &allocation, nullptr));
}

Buffer::~Buffer()
{
    if (buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(VulkanContext::getContext().getAllocator(), buffer, allocation);
    }
}

void Buffer::uploadData(const void *data, VkDeviceSize size)
{
    Buffer stagingBuffer(size,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                         VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                             VMA_ALLOCATION_CREATE_MAPPED_BIT);

    auto &context = VulkanContext::getContext();
    auto allocator = context.getAllocator();

    void *mappedData;
    vmaMapMemory(allocator, stagingBuffer.allocation, &mappedData);
    memcpy(mappedData, data, static_cast<size_t>(size));
    vmaUnmapMemory(allocator, stagingBuffer.allocation);

    auto cmd = context.beginSingleTimeCommands();
    VkBufferCopy copyRegion{0, 0, size};
    vkCmdCopyBuffer(cmd, stagingBuffer.buffer, buffer, 1, &copyRegion);
    context.endSingleTimeCommands(cmd);
}
