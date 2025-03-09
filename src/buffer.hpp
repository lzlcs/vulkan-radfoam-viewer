#pragma once

#include "vulkan_context.h"

class Buffer
{
public:
    Buffer() = default;
    Buffer(VkDeviceSize size, VkBufferUsageFlags usage,
           VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags memoryFlags = 0);
    ~Buffer();

    void uploadData(const void *data, VkDeviceSize size);
    void downloadData(void* data, VkDeviceSize size); 

    VkBuffer getBuffer() const { return buffer; }
    VkDeviceSize getSize() const { return size; }

// private:
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
};