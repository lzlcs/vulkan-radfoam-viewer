#pragma once

#include "vulkan_context.h"

class Buffer
{
public:
    Buffer() = default;
    Buffer(VkDeviceSize size, VkBufferUsageFlags usage,
           VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags memoryFlags = 0,
           bool hostVisible = false);
    ~Buffer();

    void uploadData(const void *data, VkDeviceSize dataSize, VkDeviceSize offset = 0);
    void downloadData(void *data, VkDeviceSize dataSize, VkDeviceSize offset = 0);

    VkBuffer getBuffer() const { return buffer; }
    VkDeviceSize getSize() const { return size; }

    // private:
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    bool hostVisible = false;
    void *mappedData = nullptr;
};