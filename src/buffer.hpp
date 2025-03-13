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

class Image {
    public:
        Image() = default;
    
        Image(VkImageType type, VkFormat format, VkExtent3D extent,
              VkImageUsageFlags usage, VmaMemoryUsage memoryUsage,
              VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
              uint32_t mipLevels = 1, uint32_t arrayLayers = 1,
              VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
    
        ~Image();
    
        // void uploadData(const void* data, VkDeviceSize dataSize,
        //                VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
        // void downloadData(void* data, VkDeviceSize dataSize);
    
        // void transitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout,
        //                      VkPipelineStageFlags srcStageMask,
        //                      VkPipelineStageFlags dstStageMask);

        VkImage getImage() const { return image; }
        VkFormat getFormat() const { return format; }
        VkExtent3D getExtent() const { return extent; }
        VkImageLayout getLayout() const { return currentLayout; }
    
    private:
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkExtent3D extent = {0, 0, 0};
        VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        uint32_t mipLevels = 1;
        bool hostVisible = false;
        void* mappedData = nullptr;
    };