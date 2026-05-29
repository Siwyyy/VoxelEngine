#pragma once
#include <vulkan/vulkan.h>

class Shader;

class GraphicsPipeline
{
public:
    GraphicsPipeline(VkDevice device, VkFormat swapchainImageFormat, VkFormat depthFormat, const Shader& vertShader, const Shader& fragShader);
    ~GraphicsPipeline();

    // Blokujemy kopiowanie
    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    [[nodiscard]] VkPipeline getPipeline() const { return m_pipeline; }
    [[nodiscard]] VkPipelineLayout getLayout() const { return m_layout; }

    void bind(VkCommandBuffer commandBuffer) const;

private:
    VkDevice m_device;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
};
