# Krok 4 - Komendy, Synchronizacja i Diagnostyka GPU

W ostatnim kroku inicjalizacji [[api/VoxelEngine|silnika graficznego Vulkan]] konfigurowane są obiekty służące do przesyłania poleceń do procesora graficznego, struktury synchronizacyjne procesor-karta graficzna, a także wbudowana diagnostyka czasu GPU i nakładka Dear ImGui.

Kod ten znajduje się w pliku [VulkanContext.cpp](file:///c:/dev/repos/VoxelEngine/src/renderer/VulkanContext.cpp) (klasy [[api/VulkanContext|VulkanContext]]).

---

## 📥 1. Pula i Bufory Komend (Command Pool & Command Buffers)

Procesor nie wysyła poleceń do karty graficznej pojedynczo. Zamiast tego zapisuje je w buforach poleceń (`VkCommandBuffer`), które są przesyłane w paczkach.

* **Pula poleceń (`VkCommandPool`)**: Rezerwuje pamięć dla buforów i powiązana jest z wybraną rodziną kolejek (Graphics Queue):
  ```cpp
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = queueFamily;
  ```
  Flaga `VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT` pozwala na wielokrotne resetowanie i ponowne nagrywanie pojedynczych buforów poleceń w każdej klatce.
* **Bufory poleceń (`m_commandBuffers`)**: Silnik alokuje dwa buforów poleceń (dla podwójnego buforowania, `MAX_FRAMES_IN_FLIGHT = 2`), po jednym dla każdej przetwarzanej klatki w locie.

---

## 🔄 2. Mechanizmy Synchronizacji (Sync Objects)

Vulkan nie posiada wbudowanej automatycznej synchronizacji. CPU i GPU działają asynchronicznie, co oznacza, że procesor musi jawnie sterować dostępem do zasobów [[vulkan/Krok_2_Swapchain|swapchaina]], aby zapobiec konfliktom odczytu-zapisu.

Silnik tworzy trzy typy struktur synchronizacyjnych:
1. **Semafory obrazu dostępne (`m_imageAvailableSemaphores`)**: Synchronizują operacje wewnątrz GPU. Informują kolejkę graficzną, że sterownik ekranu zakończył pobieranie obrazu ze [[vulkan/Krok_2_Swapchain|swapchaina]] i można na nim bezpiecznie rysować.
2. **Semafory renderowania zakończone (`m_renderFinishedSemaphores`)**: Informują silnik prezentacji (ekranu), że GPU zakończyło rysowanie klatki i obraz jest gotowy do wyświetlenia na monitorze.
3. **Płotki (Fences: `m_inFlightFences`)**: Synchronizują pracę CPU z GPU. Blokują wątek CPU na początku klatki za pomocą `vkWaitForFences`, zapobiegając nadpisaniu zasobów klatki $N$, które wciąż mogą być przetwarzane przez GPU:
   ```cpp
   vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
   vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
   ```

---

## 📥 3. Bufor Poleceń Pośrednich (Multi-Draw Indirect Buffer)

W celu obsługi wydajnego rysowania tysięcy chunków w jednym wywołaniu (technika [[02_Renderowanie_Vulkan|Multi-Draw Indirect (MDI)]]), silnik alokuje dedykowany bufor poleceń rysowania pośredniego:

* **Inicjalizacja**: Bufor `m_indirectBuffer` jest alokowany przy użyciu VMA z flagą użycia jako bufor komend pośrednich (`VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT`).
* **Mapowanie**: Aby uniknąć ciągłego narzutu ponownego mapowania pamięci w każdej klatce, stosuje się flagę `VMA_ALLOCATION_CREATE_MAPPED_BIT` z flagą zapisu `VMA_MEMORY_USAGE_CPU_TO_GPU`. Dzięki temu wskaźnik do pamięci GPU `m_indirectMappedData` jest przypisywany raz i pozostaje stale dostępny dla procesora.
* **Rozmiar**: Zarezerwowana przestrzeń pozwala na przesłanie do 30 000 komend rysowania (`sizeof(VkDrawIndexedIndirectCommand) * m_maxIndirectCommands`), co w zupełności wystarcza dla renderowania rozległego świata voxelowego.

Więcej informacji o samym rysowaniu MDI i integracji z globalnymi buforami [[api/MegaBuffer|MegaBuffer]] znajdziesz w rozdziale **[[02_Renderowanie_Vulkan|Renderowanie Vulkan]]**.

---

## ⏱️ 4. Diagnostyka wydajności GPU (`createQueryPool`)

Pomiary czasu klatki wykonywane na procesorze (np. przy użyciu `std::chrono`) są niedokładne w przypadku renderingu, ponieważ procesor graficzny wykonuje operacje asynchronicznie z opóźnieniem.

Silnik wykorzystuje **Query Pool** (`VkQueryPool`) do bezpośredniego odpytywania GPU o czas klatki:
* Inicjalizowana jest pula zapytań typu `VK_QUERY_TYPE_TIMESTAMP`.
* Na początku renderowania w buforze komend zapisywany jest znacznik czasu początkowego:
  ```cpp
  vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_queryPool, m_currentFrame * 2);
  ```
* Po narysowaniu geometrii i ImGui zapisywany jest znacznik czasu końcowego:
  ```cpp
  vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queryPool, m_currentFrame * 2 + 1);
  ```
* W kolejnej klatce CPU odczytuje różnicę wartości tych znaczników za pomocą funkcji `vkGetQueryPoolResults` i przelicza ją na milisekundy czasu renderowania na GPU (`m_gpuFrameTime`).

---

## 🖼️ 5. Nakładka Diagnostyczna ImGui (`initImGui`)

Kontekst graficzny **Dear ImGui** jest integrowany bezpośrednio z potokiem Vulkan za pomocą dedykowanego backendu w [[api/VulkanContext|VulkanContext]]:
* Inicjalizowana jest pula deskryptorów (`VkDescriptorPool`) wymagana przez ImGui do zarządzania własnymi teksturami (np. czcionkami).
* Inicjalizowane są funkcje pomocnicze GLFW: `ImGui_ImplGlfw_InitForVulkan`.
* Inicjalizowany jest potok ImGui w Vulkanie za pomocą struktury `ImGui_ImplVulkan_InitInfo` przekazującej dynamiczne formaty renderingu.
* Dynamiczne rysowanie nakładki ImGui odbywa się wewnątrz bloku renderingu klatki bezpośrednio przed wywołaniem prezentacji obrazu.
