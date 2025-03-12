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
        memcpy(static_cast<char*>(mappedData) + offset, data, dataSize);
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
    memcpy(static_cast<char*>(mappedData) + offset, data, static_cast<size_t>(dataSize));
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
        memcpy(data, static_cast<char*>(mappedData) + offset, dataSize);
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
    memcpy(data, static_cast<char*>(mappedData) + offset, dataSize);
    vmaUnmapMemory(allocator, stagingBuffer.allocation);
}