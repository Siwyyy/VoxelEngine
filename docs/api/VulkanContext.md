# Klasa VulkanContext

Klasa **`VulkanContext`** to serce silnika graficznego. Odpowiada za bezpośrednią komunikację z API Vulkan 1.3: tworzenie instancji, urządzeń logicznych i fizycznych, swapchaina, buforów komend, obiektów synchronizacji klatek oraz orkiestrację potoku renderowania z wykorzystaniem technologii **[[vulkan/Krok_4_Zasoby_i_Synchronizacja|Multi-Draw Indirect (MDI)]]** oraz **Dynamic Rendering**.

Definicja klasy znajduje się w pliku [VulkanContext.h](file:///c:/dev/repos/VoxelEngine/src/renderer/VulkanContext.h), a jej implementacja w [VulkanContext.cpp](file:///c:/dev/repos/VulkanContext.cpp).

---

## 🏗️ Kluczowe pola klasy (`VulkanContext.h`)

```cpp
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
    [[nodiscard]] VmaAllocator getAllocator() const { return m_allocator; }
    [[nodiscard]] uint32_t getDrawnChunksCount() const { return m_drawnChunksCount; }
    [[nodiscard]] float getGpuFrameTime() const { return m_gpuFrameTime; }

private:
    VkInstance m_instance                     = nullptr;
    VkSurfaceKHR m_surface                    = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice         = VK_NULL_HANDLE;
    VkDevice m_device                         = VK_NULL_HANDLE;
    VmaAllocator m_allocator                  = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue                   = VK_NULL_HANDLE;

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
    float m_gpuFrameTime    = 0.0f;

    std::unique_ptr<MegaBuffer> m_megaVertexBuffer;
    std::unique_ptr<MegaBuffer> m_megaIndexBuffer;

    VkBuffer m_indirectBuffer                = VK_NULL_HANDLE;
    VmaAllocation m_indirectBufferAllocation = VK_NULL_HANDLE;
    void* m_indirectMappedData               = nullptr;
    uint32_t m_maxIndirectCommands           = 30000;

    uint32_t m_drawnChunksCount = 0;
    // ...
};
```

---

## 🔑 Kluczowe Metody i Ich Rola

### 1. `void init(GLFWwindow* window)`
Odpowiada za sekwencyjne uruchomienie pełnego potoku inicjalizacji grafiki (zobacz szczegóły w dziale **[[vulkan/Krok_1_Inicjalizacja_Urzadzenia|Krok_1_Inicjalizacja_Urzadzenia]]**):
* Tworzy instancję, powierzchnię i urządzenia logiczne.
* Inicjalizuje swapchain, obrazy głębokości oraz ładuje shadery i tworzy [[api/GraphicsPipeline|GraphicsPipeline]].
* Tworzy pule komend i obiekty synchronizacji.
* Prealokuje globalne bufory **[[api/MegaBuffer|MegaBuffer]]** (512 MB Vertex Buffer i 256 MB Index Buffer).
* Tworzy bufor poleceń pośrednich `m_indirectBuffer` z mapowaniem pamięci na CPU w celu szybkiego przesyłania poleceń Multi-Draw Indirect.

### 2. `void drawFrame(const glm::mat4& viewMatrix, const std::vector<class Chunk*>& chunks)`
Metoda renderująca pojedynczą klatkę gry:
1. Czeka na zakończenie klatki z poprzedniego obiegu za pomocą płotka (`m_inFlightFences`).
2. Pobiera czas renderowania GPU z poprzedniej klatki za pomocą `vkGetQueryPoolResults`.
3. Pobiera kolejny wolny obraz ze swapchaina (`vkAcquireNextImageKHR`).
4. Resetuje bufor komend i wywołuje nagrywanie poleceń `recordCommandBuffer`.
5. Przesyła bufor do kolejki graficznej (`vkQueueSubmit`).
6. Prezentuje gotowy obraz na ekranie za pomocą `vkQueuePresentKHR`.

### 3. `void recordCommandBuffer(...)`
Nagrywa polecenia rysowania dla GPU:
* Zaczyna dynamiczne renderowanie (`vkCmdBeginRendering`) dla bufora koloru i bufora głębokości.
* Wiąże potok graficzny (`m_graphicsPipeline->bind`) oraz konfiguruje dynamiczny Viewport i Scissor.
* Wylicza macierz $MVP$ (Model-View-Projection) z uwzględnieniem skali wokseli ($0.05$) i przekazuje ją za pomocą Push Constants (`vkCmdPushConstants`).
* Wiąże globalne buforowanie wierzchołków i indeksów z [[api/MegaBuffer|MegaBuffer]].
* Iteruje po liście chunków: filtruje je przez [[algorithms/Frustum_Culling|bryłę widoku (Frustum Culling)]] i generuje dla każdego widocznego [[api/Chunk|chunku]] strukturę `VkDrawIndexedIndirectCommand`.
* Kopiuje polecenia do zmapowanego bufora komend pośrednich i wykonuje **jeden draw call**:
  ```cpp
  vkCmdDrawIndexedIndirect(commandBuffer, m_indirectBuffer, 0, indirectCommandsCount, sizeof(VkDrawIndexedIndirectCommand));
  ```
* Renderuje GUI ImGui i kończy renderowanie.

---

## 🔗 Relacje z innymi klasami skarbca

* **[[api/MegaBuffer|MegaBuffer]]** — kontekst posiada dwa globalne mega-bufory wierzchołków i indeksów.
* **[[api/GraphicsPipeline|GraphicsPipeline]]** — zarządza stanem potoku renderowania wokseli.
* **[[api/World|World]]** — dostarcza listę aktywnych chunków w celu przefiltrowania i narysowania.
* **[[api/Frustum|Frustum]]** — wywoływana do frustum cullingu przed zapisaniem komendy MDI dla GPU.
* **[[vulkan/Krok_1_Inicjalizacja_Urzadzenia|Krok_1_Inicjalizacja_Urzadzenia]]** — seria notatek szczegółowo opisująca proces uruchamiania VulkanContext.
