# ⚡ Renderowanie Vulkan

Silnik graficzny **VoxelEngine** wykorzystuje interfejs API **Vulkan 1.3**. Został zaprojektowany z myślą o maksymalnej wydajności renderowania przy jednoczesnym ograniczeniu narzutu sterownika (overhead) na procesor. 

Głównym punktem wejścia silnika graficznego jest klasa [[api/VulkanContext|VulkanContext]].

---

## 🎨 1. Dynamic Rendering (Vulkan 1.3)

Silnik rezygnuje z tradycyjnych mechanizmów Vulkan, takich jak *Render Passes* i *Framebuffers*, na rzecz rozszerzenia **Dynamic Rendering** (`VK_KHR_dynamic_rendering`). 

### Korzyści:
* **Uproszczona inicjalizacja**: Potok graficzny ([[api/GraphicsPipeline|GraphicsPipeline]]) nie musi być powiązany z konkretnym Render Passem podczas tworzenia.
* **Mniejszy narzut pamięciowy**: Brak konieczności tworzenia obiektów typu framebuffer dla każdego obrazu swapchaina.
* **Elastyczność**: W metodzie `recordCommandBuffer` rysowanie rozpoczyna się od bezpośredniego wywołania struktury `VkRenderingInfo` za pomocą funkcji `vkCmdBeginRendering`:

```cpp
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
```

---

## 🚀 2. Technika Multi-Draw Indirect (MDI)

Najważniejszą optymalizacją w VoxelEngine jest zastosowanie **Multi-Draw Indirect** (`multiDrawIndirect` w funkcjach urządzenia). Umożliwia to narysowanie wszystkich aktywnych chunków za pomocą **dokładnie jednego wywołania draw call na GPU**.

### Jak to działa?
1. **Bufor komend pośrednich (`m_indirectBuffer`)**: W pamięci karty graficznej prealokowany jest bufor o rozmiarze `m_maxIndirectCommands` (domyślnie 30 000 komend) typu `VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT`. Bufor ten jest stale zmapowany (`m_indirectMappedData`).
2. **Generowanie listy poleceń na CPU**: Dla każdego [[api/Chunk|chunku]], który przeszedł [[algorithms/Frustum_Culling|test bryły widoku (frustum culling)]], CPU zapisuje strukturę `VkDrawIndexedIndirectCommand`:
    ```cpp
    VkDrawIndexedIndirectCommand cmd{
        .indexCount    = chunk->getIndexCount(), // Liczba indeksów siatki chunku
        .instanceCount = 1,
        .firstIndex    = static_cast<uint32_t>(chunk->getIndexOffset() / sizeof(uint32_t)),
        .vertexOffset  = static_cast<int32_t>(chunk->getVertexOffset() / sizeof(Vertex)),
        .firstInstance = 0
    };
    ```
3. **Kopiowanie**: Wszystkie polecenia są kopiowane za pomocą `std::memcpy` bezpośrednio do pamięci zmapowanej.
4. **Wywołanie na GPU**: Następuje jedno wywołanie systemowe rysujące:
   ```cpp
   vkCmdDrawIndexedIndirect(commandBuffer, m_indirectBuffer, 0, 
                            static_cast<uint32_t>(indirectCommands.size()), 
                            sizeof(VkDrawIndexedIndirectCommand));
   ```

Dzięki temu narzut sterownika graficznego na procesor jest niezmienny bez względu na to, czy na ekranie znajduje się 10 czy 10 000 [[api/Chunk|chunków]].

---

## 💾 3. System Alokacji Pamięci: [[api/MegaBuffer|MegaBuffer]]

Tradycyjne tworzenie osobnych buforów wierzchołków (Vertex Buffer) i indeksów (Index Buffer) dla każdego [[api/Chunk|chunku]] prowadziłoby do tysięcy alokacji pamięci na GPU, co drastycznie spowolniłoby aplikację i doprowadziło do fragmentacji VRAM.

Silnik wprowadza koncepcję **[[api/MegaBuffer|MegaBuffer]]** – globalnej puli pamięci:

```mermaid
gantt
    title Wizualizacja Pamięci w MegaBuffer
    dateFormat  HH:mm
    axisFormat %H:%M
    section MegaVertexBuffer
    Chunk 1 (Offset: 0, Rozmiar: 12KB) :active, c1, 00:00, 00:10
    Wolna przestrzeń : c2, 00:10, 00:15
    Chunk 2 (Offset: 18KB, Rozmiar: 24KB) :active, c3, 00:15, 00:35
    Chunk 3 (Offset: 42KB, Rozmiar: 8KB) :active, c4, 00:35, 00:45
    Wolna przestrzeń : c5, 00:45, 01:00
```

### Specyfikacja techniczna [[api/MegaBuffer|MegaBuffer]]:
* **Prealokacja**: W [[api/VulkanContext|VulkanContext]] inicjalizowane są dwa duże mega-bufory przy użyciu biblioteki **Vulkan Memory Allocator (VMA)**:
  * `m_megaVertexBuffer` (`VK_BUFFER_USAGE_VERTEX_BUFFER_BIT`)
  * `m_megaIndexBuffer` (`VK_BUFFER_USAGE_INDEX_BUFFER_BIT`)
* **Custom Allocator**: Klasa [[api/MegaBuffer|MegaBuffer]] zarządza wolną przestrzenią za pomocą listy struktur `FreeBlock` (przechowujących offset i rozmiar).
* **Alokacja**: Gdy [[api/Chunk|chunk]] przebudowuje swoją siatkę (mesh), prosi o przydzielenie wolnego obszaru:
  ```cpp
  BlockAllocation allocate(uint32_t size);
  ```
  Zwracana jest struktura `BlockAllocation` posiadająca przesunięcie (`offset`) oraz status walidacji.
* **Uwalnianie pamięci**: Gdy [[api/Chunk|chunk]] jest usuwany lub regenerowany, wywoływana jest funkcja `free(BlockAllocation)`. Sąsiednie wolne bloki są ze sobą scalane w celu zapobiegania fragmentacji.
* **Przesyłanie danych**: Funkcja `upload()` wykonuje bezpośrednią operację kopiowania danych wierzchołków do pamięci host-visible zmapowanego bufora.

---

## ⚙️ 4. Potok Renderowania ([[api/GraphicsPipeline|GraphicsPipeline]])

Klasa [[api/GraphicsPipeline|GraphicsPipeline]] konfiguruje stan potoku graficznego:
* **Shadery**: Kompilowane z kodu GLSL do formatu binarnego SPIR-V za pomocą kompilatora `glslc`.
  * Shader wierzchołków (`shaders/shader.vert`): przetwarza wierzchołki, nakłada transformacje widoku-projekcji (MVP) przekazywane przez obiekt `Push Constants`.
  * Shader fragmentów (`shaders/shader.frag`): oblicza kolor na podstawie atrybutów wierzchołków oraz nakłada proste oświetlenie kierunkowe oparte na normalnych.
* **Format Wierzchołka (`Vertex`)**:
  * Pozycja: `glm::vec3` (powiązana z `location = 0` w shaderze).
  * Kolor: `glm::vec3` (powiązana z `location = 1` w shaderze).
* **Push Constants**: Do przekazywania macierzy MVP bezpośrednio z CPU do GPU w czasie rzeczywistym bez narzutu na tworzenie Descriptor Setów (macierz pobiera dane z klasy [[api/Time|Time]]).
* **Multisampling & Depth Stencil**: Włączony bufor głębokości (Depth Buffer) o formacie `VK_FORMAT_D32_SFLOAT`, zapobiegający błędnemu nakładaniu się ścian voxelowych.

---

> [!NOTE]
> Synchronizacja klatek realizowana jest za pomocą technologii podwójnego buforowania (`MAX_FRAMES_IN_FLIGHT = 2`). Używane są dwa zestawy płotków (Fences) oraz semaforów, co pozwala procesorowi na przygotowywanie klatki `N+1`, podczas gdy procesor graficzny kończy rysowanie klatki `N`.
