#include "VulkanContext.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <print>
#include <set>
#include <stdexcept>
#include <string>

#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include "GraphicsPipeline.h"
#include "Shader.h"
#include "core/Frustum.h"
#include "utils/FileSystem.h"
#include "world/Chunk.h"

namespace voxl
{
    namespace
    {
        VkResult createDebugUtilsMessengerEXT(VkInstance instance,
                                              const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                              // ReSharper disable once CppDFAConstantParameter
                                              const VkAllocationCallbacks* pAllocator,
                                              VkDebugUtilsMessengerEXT* pDebugMessenger)
        {
            if (const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
                    instance, "vkCreateDebugUtilsMessengerEXT")); func
                != nullptr)
            {
                return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
            }
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }

        void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                           // ReSharper disable once CppDFAConstantParameter
                                           const VkAllocationCallbacks* pAllocator)
        {
            if (const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")); func != nullptr)
            {
                func(instance, debugMessenger, pAllocator);
            }
        }
    }

    VulkanContext::VulkanContext() = default;

    VulkanContext::~VulkanContext() = default;

    void VulkanContext::init(GLFWwindow* window)
    {
        createInstance();
        setupDebugMessenger();
        createSurface(window);
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapchain(window);
        createImageViews();
        createDepthResources();

        Shader vertShader(m_device, "shaders/shader.vert.spv");
        Shader fragShader(m_device, "shaders/shader.frag.spv");
        m_graphicsPipeline = std::make_unique<GraphicsPipeline>(m_device, m_swapchainImageFormat, m_depthFormat,
                                                                vertShader, fragShader);

        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
        createQueryPool();

        VkDeviceSize vertexMegaSize = 512ULL * 1024 * 1024; // 512 MB
        VkDeviceSize indexMegaSize  = 256ULL * 1024 * 1024; // 256 MB
        m_megaVertexBuffer          = std::make_unique<MegaBuffer>(m_allocator, vertexMegaSize,
                                                                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        m_megaIndexBuffer = std::make_unique<MegaBuffer>(m_allocator, indexMegaSize,
                                                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

        const VkBufferCreateInfo indirectInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(VkDrawIndexedIndirectCommand) * m_maxIndirectCommands,
            .usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        constexpr VmaAllocationCreateInfo indirectAllocInfo{
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
        };

        VmaAllocationInfo allocInfo;
        vmaCreateBuffer(m_allocator, &indirectInfo, &indirectAllocInfo, &m_indirectBuffer, &m_indirectBufferAllocation,
                        &allocInfo);
        m_indirectMappedData = allocInfo.pMappedData;

        initImGui(window);
    }

    void VulkanContext::cleanup()
    {
        cleanupImGui();

        vkDeviceWaitIdle(m_device);

        for (const auto semaphore: m_renderFinishedSemaphores)
        {
            vkDestroySemaphore(m_device, semaphore, nullptr);
        }
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(m_device, m_commandPool, nullptr);

        if (m_queryPool != VK_NULL_HANDLE)
        {
            vkDestroyQueryPool(m_device, m_queryPool, nullptr);
        }

        m_megaVertexBuffer.reset();
        m_megaIndexBuffer.reset();
        if (m_indirectBuffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(m_allocator, m_indirectBuffer, m_indirectBufferAllocation);
        }

        m_graphicsPipeline.reset();

        if (m_depthImage != VK_NULL_HANDLE)
        {
            vmaDestroyImage(m_allocator, m_depthImage, m_depthImageAllocation);
            vkDestroyImageView(m_device, m_depthImageView, nullptr);
        }

        vmaDestroyAllocator(m_allocator);

        for (const auto imageView: m_swapchainImageViews)
        {
            vkDestroyImageView(m_device, imageView, nullptr);
        }
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

        vkDestroyDevice(m_device, nullptr);

        if (ENABLE_VALIDATION_LAYERS)
        {
            destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyInstance(m_instance, nullptr);
    }

    void VulkanContext::deviceWaitIdle() const
    {
        vkDeviceWaitIdle(m_device);
    }

    void VulkanContext::drawFrame(const glm::mat4& viewMatrix, const std::vector<Chunk*>& chunks)
    {
        vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

        if (!m_firstFrame[m_currentFrame])
        {
            std::array<uint64_t, 2> timestamps{};
            if (vkGetQueryPoolResults(m_device, m_queryPool, m_currentFrame * 2, 2, timestamps.size() * sizeof(uint64_t),
                                      timestamps.data(),
                                      sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS)
            {
                m_gpuFrameTime = static_cast<float>(timestamps[1] - timestamps[0]) * m_timestampPeriod * 1e-6f;
            }
        }
        m_firstFrame[m_currentFrame] = false;

        vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE,
                              &imageIndex);

        vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);
        recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex, viewMatrix, chunks);

        const std::array waitSemaphores                          = {m_imageAvailableSemaphores[m_currentFrame]};
        constexpr std::array<VkPipelineStageFlags, 1> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        const std::array signalSemaphores                        = {m_renderFinishedSemaphores[imageIndex]};

        const VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = waitSemaphores.data(),
            .pWaitDstStageMask = waitStages.data(),
            .commandBufferCount = 1,
            .pCommandBuffers = &m_commandBuffers[m_currentFrame],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = signalSemaphores.data()
        };

        if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        const std::array swapchains = {m_swapchain};
        const VkPresentInfoKHR presentInfo{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = signalSemaphores.data(),
            .swapchainCount = 1,
            .pSwapchains = swapchains.data(),
            .pImageIndices = &imageIndex
        };

        vkQueuePresentKHR(m_presentQueue, &presentInfo);

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void VulkanContext::initImGui(GLFWwindow* window) const
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        const ImGuiIO& io = ImGui::GetIO();
        (void)io;
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(window, true);

        ImGui_ImplVulkan_InitInfo init_info = {
            .Instance = m_instance,
            .PhysicalDevice = m_physicalDevice,
            .Device = m_device,
            .QueueFamily = findQueueFamilies(m_physicalDevice).graphicsFamily.value_or(0),
            .Queue = m_graphicsQueue,
            .DescriptorPoolSize = 1000,
            .MinImageCount = MAX_FRAMES_IN_FLIGHT,
            .ImageCount = MAX_FRAMES_IN_FLIGHT,
            .PipelineInfoMain = ImGui_ImplVulkan_PipelineInfo{
                .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
                .PipelineRenderingCreateInfo = VkPipelineRenderingCreateInfo{
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                    .colorAttachmentCount = 1,
                    .pColorAttachmentFormats = &m_swapchainImageFormat,
                    .depthAttachmentFormat = m_depthFormat
                }
            },
            .UseDynamicRendering = true
        };

        ImGui_ImplVulkan_Init(&init_info);
    }

    void VulkanContext::cleanupImGui()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void VulkanContext::renderImGui(VkCommandBuffer commandBuffer)
    {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }

    void VulkanContext::createInstance()
    {
        if constexpr (ENABLE_VALIDATION_LAYERS)
        {
            if (!checkValidationLayerSupport())
            {
                throw std::runtime_error("Validation layers requested, but not available!");
            }
        }

        VkApplicationInfo appInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Voxel Engine",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "Voxel Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_3
        };

        const auto extensions = getRequiredExtensions();
        VkInstanceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data()
        };

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if constexpr (ENABLE_VALIDATION_LAYERS)
        {
            createInfo.enabledLayerCount   = static_cast<uint32_t>(VALIDATION_LAYERS.size());
            createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = &debugCreateInfo;
        }

        if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan instance!");
        }

        std::println("Success: Vulkan instance and debug messenger created!");
    }

    void VulkanContext::setupDebugMessenger()
    {
        if constexpr (!ENABLE_VALIDATION_LAYERS)
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (createDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to set up debug messenger!");
        }
    }

    void VulkanContext::createSurface(GLFWwindow* window)
    {
        if (glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!");
        }
    }

    void VulkanContext::pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

        if (deviceCount == 0)
        {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

        for (const auto& device: devices)
        {
            if (isDeviceSuitable(device))
            {
                m_physicalDevice = device;
                break;
            }
        }

        if (m_physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    void VulkanContext::createLogicalDevice()
    {
        auto [graphicsFamily, presentFamily] = findQueueFamilies(m_physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set uniqueQueueFamilies = {graphicsFamily.value_or(0), presentFamily.value_or(0)};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily: uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queueFamily,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority
            };
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.multiDrawIndirect = VK_TRUE;

        VkPhysicalDeviceVulkan13Features vulkan13Features{};
        vulkan13Features.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        vulkan13Features.dynamicRendering = VK_TRUE;

        VkDeviceCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &vulkan13Features,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),
            .enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size()),
            .ppEnabledExtensionNames = DEVICE_EXTENSIONS.data(),
            .pEnabledFeatures = &deviceFeatures
        };

        if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create logical device!");
        }

        vkGetDeviceQueue(m_device, graphicsFamily.value_or(0), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, presentFamily.value_or(0), 0, &m_presentQueue);

        VmaAllocatorCreateInfo allocatorInfo{
            .physicalDevice = m_physicalDevice,
            .device = m_device,
            .instance = m_instance,
            .vulkanApiVersion = VK_API_VERSION_1_3
        };
        if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create VMA allocator!");
        }
    }

    void VulkanContext::createSwapchain(GLFWwindow* window)
    {
        const auto [capabilities, formats, presentModes] = querySwapChainSupport(m_physicalDevice);

        const auto [format, colorSpace]    = chooseSwapSurfaceFormat(formats);
        const VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
        const VkExtent2D extent            = chooseSwapExtent(capabilities, window);

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        {
            imageCount = capabilities.maxImageCount;
        }

        const auto [graphicsFamily, presentFamily] = findQueueFamilies(m_physicalDevice);
        const std::array queueFamilyIndices        = {
            graphicsFamily.value_or(0), presentFamily.value_or(0)
        };

        const VkSwapchainCreateInfoKHR createInfo{
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = m_surface,
            .minImageCount = imageCount,
            .imageFormat = format,
            .imageColorSpace = colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = (graphicsFamily != presentFamily)
                                    ? VK_SHARING_MODE_CONCURRENT
                                    : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = (graphicsFamily != presentFamily) ? 2u : 0u,
            .pQueueFamilyIndices = (graphicsFamily != presentFamily)
                                       ? queueFamilyIndices.data()
                                       : nullptr,
            .preTransform = capabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = presentMode,
            .clipped = VK_TRUE,
            .oldSwapchain = VK_NULL_HANDLE
        };

        if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
        m_swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());

        m_swapchainImageFormat = format;
        m_swapchainExtent      = extent;
    }

    void VulkanContext::createImageViews()
    {
        m_swapchainImageViews.resize(m_swapchainImages.size());

        for (size_t i = 0; i < m_swapchainImages.size(); i++)
        {
            VkImageViewCreateInfo createInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = m_swapchainImages[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = m_swapchainImageFormat,
                .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY
                },
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };

            if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create image views!");
            }
        }
    }

    void VulkanContext::createDepthResources()
    {
        m_depthFormat = findDepthFormat();

        const VkImageCreateInfo imageInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = m_depthFormat,
            .extent = {
                .width = m_swapchainExtent.width,
                .height = m_swapchainExtent.height,
                .depth = 1
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
        };

        constexpr VmaAllocationCreateInfo allocInfo{
            .usage = VMA_MEMORY_USAGE_GPU_ONLY
        };

        if (vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &m_depthImage, &m_depthImageAllocation, nullptr) !=
            VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create depth image!");
        }

        const VkImageViewCreateInfo viewInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_depthImage,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = m_depthFormat,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_depthImageView) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create depth image view!");
        }
    }

    void VulkanContext::createCommandPool()
    {
        const QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

        const VkCommandPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value_or(0)
        };

        if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create command pool!");
        }
    }

    void VulkanContext::createCommandBuffers()
    {
        m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        const VkCommandBufferAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size())
        };

        if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffers!");
        }
    }

    void VulkanContext::createSyncObjects()
    {
        m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(m_swapchainImageViews.size());
        m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        constexpr VkSemaphoreCreateInfo semaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        constexpr VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create synchronization objects for a frame!");
            }
        }

        for (size_t i = 0; i < m_swapchainImageViews.size(); i++)
        {
            if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create render finished semaphores!");
            }
        }
    }

    void VulkanContext::createQueryPool()
    {
        constexpr VkQueryPoolCreateInfo queryPoolInfo{
            .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
            .queryType = VK_QUERY_TYPE_TIMESTAMP,
            .queryCount = 2 * 2 // MAX_FRAMES_IN_FLIGHT * 2
        };

        if (vkCreateQueryPool(m_device, &queryPoolInfo, nullptr, &m_queryPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create query pool!");
        }

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);
        m_timestampPeriod = deviceProperties.limits.timestampPeriod;
    }

    void VulkanContext::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex,
                                            const glm::mat4& viewMatrix,
                                            const std::vector<Chunk*>& chunks)
    {
        VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
        };

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        vkCmdResetQueryPool(commandBuffer, m_queryPool, m_currentFrame * 2, 2);
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_queryPool, m_currentFrame * 2);

        VkImageMemoryBarrier imageMemoryBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .image = m_swapchainImages[imageIndex],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &imageMemoryBarrier);

        VkImageMemoryBarrier depthBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_depthImage,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &depthBarrier);

        VkRenderingAttachmentInfo colorAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_swapchainImageViews[imageIndex],
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {
                .color = {{0.0f, 0.0f, 0.0f, 1.0f}}
            }
        };

        VkRenderingAttachmentInfo depthAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = m_depthImageView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue = {
                .depthStencil = {.depth = 1.0f, .stencil = 0}
            }
        };

        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {
                .offset = {.x = 0, .y = 0},
                .extent = m_swapchainExtent
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentInfo,
            .pDepthAttachment = &depthAttachmentInfo
        };

        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        m_graphicsPipeline->bind(commandBuffer);

        VkViewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(m_swapchainExtent.width),
            .height = static_cast<float>(m_swapchainExtent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{
            .offset = {.x = 0, .y = 0},
            .extent = m_swapchainExtent
        };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        glm::mat4 projOriginal = glm::perspective(glm::radians(45.0f),
                                                  static_cast<float>(m_swapchainExtent.width) / static_cast<float>(
                                                      m_swapchainExtent.height),
                                                  0.1f, 100.0f);
        glm::mat4 proj = projOriginal;
        proj[1][1]     *= -1;

        glm::mat4 vpOriginal = projOriginal * viewMatrix;
        Frustum frustum(vpOriginal);

        glm::mat4 vp    = proj * viewMatrix;
        glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));
        glm::mat4 mvp   = vp * model;

        vkCmdPushConstants(commandBuffer, m_graphicsPipeline->getLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(glm::mat4), &mvp);

        std::array<VkBuffer, 1> vertexBuffers = {m_megaVertexBuffer->getBuffer()};
        std::array<VkDeviceSize, 1> offsets   = {};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
        vkCmdBindIndexBuffer(commandBuffer, m_megaIndexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

        m_drawnChunksCount = 0;
        std::vector<VkDrawIndexedIndirectCommand> indirectCommands;

        for (const auto& chunk: chunks)
        {
            glm::vec3 minAABB = (chunk->getPosition() - glm::vec3(0.5f)) * 0.05f;
            glm::vec3 maxAABB = (chunk->getPosition() + glm::vec3(Chunk::CHUNK_SIZE - 0.5f)) * 0.05f;

            if (!frustum.intersectsAABB(minAABB, maxAABB))
            {
                continue;
            }

            if (chunk->getIndexCount() == 0 || !chunk->hasValidAllocation()) continue;

            VkDrawIndexedIndirectCommand cmd{
                .indexCount = chunk->getIndexCount(),
                .instanceCount = 1,
                .firstIndex = static_cast<uint32_t>(chunk->getIndexOffset() / sizeof(uint32_t)),
                .vertexOffset = static_cast<int32_t>(chunk->getVertexOffset() / sizeof(Vertex)),
                .firstInstance = 0
            };

            indirectCommands.push_back(cmd);
            m_drawnChunksCount++;
        }

        if (!indirectCommands.empty())
        {
            std::memcpy(m_indirectMappedData, indirectCommands.data(),
                        indirectCommands.size() * sizeof(VkDrawIndexedIndirectCommand));
            vkCmdDrawIndexedIndirect(commandBuffer, m_indirectBuffer, 0, static_cast<uint32_t>(indirectCommands.size()),
                                     sizeof(VkDrawIndexedIndirectCommand));
        }

        renderImGui(commandBuffer);

        vkCmdEndRendering(commandBuffer);

        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queryPool, m_currentFrame * 2 + 1);

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = 0;
        imageMemoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imageMemoryBarrier.newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }

    bool VulkanContext::isDeviceSuitable(VkPhysicalDevice device) const
    {
        const QueueFamilyIndices indices = findQueueFamilies(device);
        const bool extensionsSupported   = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate                              = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        const bool supportsVulkan13 = deviceProperties.apiVersion >= VK_API_VERSION_1_3;

        return indices.isComplete() && extensionsSupported && swapChainAdequate && supportsVulkan13;
    }

    bool VulkanContext::checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

        for (const auto& extension: availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    QueueFamilyIndices VulkanContext::findQueueFamilies(VkPhysicalDevice device) const
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily: queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

            if (presentSupport)
            {
                indices.presentFamily = i;
            }

            if (indices.isComplete())
            {
                break;
            }

            i++;
        }

        return indices;
    }

    uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type!");
    }

    VkFormat VulkanContext::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                                VkFormatFeatureFlags features) const
    {
        for (const VkFormat format: candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

            if ((tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) ||
                (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features))
            {
                return format;
            }
        }
        throw std::runtime_error("Failed to find supported format!");
    }

    VkFormat VulkanContext::findDepthFormat() const
    {
        return findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    SwapChainSupportDetails VulkanContext::querySwapChainSupport(VkPhysicalDevice device) const
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount,
                                                      details.presentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR VulkanContext::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& availableFormat: availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace ==
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR VulkanContext::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
        for (const auto& availablePresentMode: availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanContext::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);

        return actualExtent;
    }

    bool VulkanContext::checkValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName: VALIDATION_LAYERS)
        {
            bool layerFound = false;

            for (const auto& layerProperties: availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }

        return true;
    }

    std::vector<const char*> VulkanContext::getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (ENABLE_VALIDATION_LAYERS)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    void VulkanContext::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = VkDebugUtilsMessengerCreateInfoEXT{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debugCallback
        };
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        std::println(stderr, "Validation layer: {}", pCallbackData->pMessage);
        return VK_FALSE;
    }
} // namespace voxl
