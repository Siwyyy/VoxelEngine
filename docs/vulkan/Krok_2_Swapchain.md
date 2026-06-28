# Krok 2 - Konfiguracja Swapchaina i Bufora Głębokości

W tym kroku [[api/VoxelEngine|silnik graficzny]] inicjalizuje mechanizmy wyświetlania klatek: Swapchain (łańcuch wymiany obrazów), widoki obrazów (Image Views) oraz alokuje bufor głębokości (Depth Buffer) przy użyciu Vulkan Memory Allocator (VMA).

Kod ten znajduje się w pliku [VulkanContext.cpp](../../src/renderer/VulkanContext.cpp) (klasy [[api/VulkanContext|VulkanContext]]).

---

## 🔁 1. Tworzenie Łańcucha Wymiany (`createSwapchain`)

Swapchain (`VkSwapchainKHR`) to kolejka buforów obrazu przechowywana na GPU. Odpowiada za synchronizację renderowania z odświeżaniem monitora (zapobieganie tearingowi obrazu).

Podczas tworzenia swapchaina silnik wybiera trzy kluczowe parametry:
1. **Format powierzchni (`chooseSwapSurfaceFormat`)**: Silnik szuka formatu **`VK_FORMAT_B8G8R8A8_SRGB`** z przestrzenią kolorów **`VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`** dla uzyskania poprawnych barw sRGB.
2. **Tryb prezentacji (`chooseSwapPresentMode`)**: Silnik preferuje tryb **`VK_PRESENT_MODE_MAILBOX_KHR`** (tzw. potrójne buforowanie, oferujące najniższy input lag bez tearingu) z fallbackiem do trybu **`VK_PRESENT_MODE_FIFO_KHR`** (standardowy VSync).
3. **Rozdzielczość (`chooseSwapExtent`)**: Dopasowuje rozmiar buforów swapchaina (w pikselach) do aktualnego rozmiaru [[api/Window|okna GLFW]].

Po utworzeniu pobierane są uchwyty do obrazów swapchaina (`m_swapchainImages`), które zostaną użyte jako cele renderowania.

```cpp
VkSwapchainCreateInfoKHR createInfo{};
createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
createInfo.surface = m_surface;
createInfo.minImageCount    = imageCount;
createInfo.imageFormat      = surfaceFormat.format;
createInfo.imageExtent      = extent;
createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
// ...
vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain);
```

---

## 👁️ 2. Tworzenie Widoków Obrazu (`createImageViews`)

Obiekt typu `VkImage` w Vulkanie reprezentuje czysty bufor pikseli w pamięci GPU. Silnik nie może rysować bezpośrednio na nim – musi opakować go w widok obrazu `VkImageView`.

Dla każdego obrazu w swapchainie tworzony jest widok:
* **`viewType`**: Ustawiony na `VK_IMAGE_VIEW_TYPE_2D` (tekstura dwuwymiarowa).
* **`format`**: Zgodny z formatem obrazów swapchaina.
* **`subresourceRange.aspectMask`**: Ustawiona na `VK_IMAGE_ASPECT_COLOR_BIT` (reprezentuje dane koloru).

---

## 🏔️ 3. Bufor Głębokości (`createDepthResources`)

[[api/VoxelEngine|VoxelEngine]] renderuje trójwymiarową przestrzeń wokseli. Aby bloki położone z tyłu nie były rysowane na blokach znajdujących się z przodu, potrzebny jest bufor głębokości (Depth Buffer).

Inicjalizacja bufora głębokości przebiega w trzech krokach:
1. **Wybór formatu**: Silnik sprawdza sprzętową obsługę formatów o wysokiej precyzji, szukając w pierwszej kolejności formatu **`VK_FORMAT_D32_SFLOAT`** (32-bitowa głębokość zmiennoprzecinkowa).
2. **Alokacja obrazu za pomocą VMA**: Obraz bufora głębokości jest alokowany bezpośrednio w pamięci VRAM karty graficznej (`VMA_MEMORY_USAGE_GPU_ONLY`) z flagą użycia jako załącznik głębokości:
   ```cpp
   VkImageCreateInfo imageInfo{};
   imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
   imageInfo.imageType     = VK_IMAGE_TYPE_2D;
   imageInfo.format        = m_depthFormat;
   imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
   // ...
   vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &m_depthImage, &m_depthImageAllocation, nullptr);
   ```
3. **Tworzenie widoku głębokości (`VkImageView`)**: Obraz głębokości jest opakowywany w widok z flagą `subresourceRange.aspectMask` ustawioną na **`VK_IMAGE_ASPECT_DEPTH_BIT`**.
