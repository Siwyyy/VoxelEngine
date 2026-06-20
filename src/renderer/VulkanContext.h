#pragma once

#include <memory>
#include <optional>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vma/vk_mem_alloc.h>

#include "Buffer.h"
#include "MegaBuffer.h"

class Chunk;

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    [[nodiscard]] bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

const std::vector VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};

const std::vector DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

class VulkanContext
{
public:
    VulkanContext();
    ~VulkanContext();

    void init(GLFWwindow* window);
    void cleanup();
    void deviceWaitIdle() const;
    void drawFrame(const glm::mat4& viewMatrix, const std::vector<Chunk*>& chunks);

    void initImGui(GLFWwindow* window) const;
    void cleanupImGui();
    void renderImGui(VkCommandBuffer commandBuffer);

    [[nodiscard]] VkDevice getDevice() const { return m_device; }
    [[nodiscard]] VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    [[nodiscard]] VmaAllocator getAllocator() const { return m_allocator; }

    [[nodiscard]] uint32_t getDrawnChunksCount() const { return m_drawnChunksCount; }
    [[nodiscard]] float getGpuFrameTime() const { return m_gpuFrameTime; }

    [[nodiscard]] MegaBuffer* getMegaVertexBuffer() const { return m_megaVertexBuffer.get(); }
    [[nodiscard]] MegaBuffer* getMegaIndexBuffer() const { return m_megaIndexBuffer.get(); }

private:
    VkInstance m_instance                     = nullptr;
    VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;
    VkSurfaceKHR m_surface                    = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice         = VK_NULL_HANDLE;
    VkDevice m_device                         = VK_NULL_HANDLE;
    VmaAllocator m_allocator                  = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue                   = VK_NULL_HANDLE;
    VkQueue m_presentQueue                    = VK_NULL_HANDLE;

    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_swapchainImages;
    VkFormat m_swapchainImageFormat{};
    VkExtent2D m_swapchainExtent{};
    std::vector<VkImageView> m_swapchainImageViews;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_commandBuffers;

    VkImage m_depthImage                 = VK_NULL_HANDLE;
    VmaAllocation m_depthImageAllocation = VK_NULL_HANDLE;
    VkImageView m_depthImageView         = VK_NULL_HANDLE;
    VkFormat m_depthFormat{};

    std::unique_ptr<class GraphicsPipeline> m_graphicsPipeline;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    uint32_t m_currentFrame = 0;

    VkQueryPool m_queryPool = VK_NULL_HANDLE;
    float m_timestampPeriod = 1.0f;
    float m_gpuFrameTime    = 0.0f;
    bool m_firstFrame[2]    = {true, true};

    std::unique_ptr<MegaBuffer> m_megaVertexBuffer;
    std::unique_ptr<MegaBuffer> m_megaIndexBuffer;

    VkBuffer m_indirectBuffer                = VK_NULL_HANDLE;
    VmaAllocation m_indirectBufferAllocation = VK_NULL_HANDLE;
    void* m_indirectMappedData               = nullptr;
    uint32_t m_maxIndirectCommands           = 30000;

    uint32_t m_drawnChunksCount = 0;

    void createInstance();
    void setupDebugMessenger();
    void createSurface(GLFWwindow* window);
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain(GLFWwindow* window);
    void createImageViews();
    void createDepthResources();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void createQueryPool();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, const glm::mat4& viewMatrix,
                             const std::vector<Chunk*>& chunks);

    [[nodiscard]] bool isDeviceSuitable(VkPhysicalDevice device) const;
    [[nodiscard]] bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;
    [[nodiscard]] QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

    [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    [[nodiscard]] VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                               VkFormatFeatureFlags features) const;
    [[nodiscard]] VkFormat findDepthFormat() const;

    [[nodiscard]] SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;
    [[nodiscard]] VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    [[nodiscard]] VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    [[nodiscard]] VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) const;

    [[nodiscard]] bool checkValidationLayerSupport() const;
    [[nodiscard]] std::vector<const char*> getRequiredExtensions() const;
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};
