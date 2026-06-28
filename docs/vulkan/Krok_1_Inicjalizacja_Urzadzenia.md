# Krok 1 - Inicjalizacja Urządzenia Vulkan

Pierwszy krok inicjalizacji [[api/VoxelEngine|silnika graficznego]] polega na skonfigurowaniu bazowego środowiska Vulkan: utworzeniu instancji, powiązaniu debuggowania, powiązaniu [[api/Window|okna GLFW]] oraz utworzeniu logicznego urządzenia GPU obsługującego wymagane rozszerzenia.

Kod ten znajduje się w pliku [VulkanContext.cpp](../../src/renderer/VulkanContext.cpp) (klasy [[api/VulkanContext|VulkanContext]]).

---

## 🏗️ 1. Tworzenie Instancji (`createInstance`)

Instancja (`VkInstance`) jest punktem wejścia aplikacji do API Vulkan. Inicjalizuje sterownik i przechowuje stan aplikacji.

Podczas tworzenia instancji silnik konfiguruje:
* **Metadane aplikacji (`VkApplicationInfo`)**: Nazwa silnika, wersje aplikacji i wymagana wersja API Vulkan (ustawiona na **Vulkan 1.3**).
* **Warstwy Walidacyjne (Validation Layers)**: Jeśli aplikacja jest w trybie debugowania (`ENABLE_VALIDATION_LAYERS = true`), żądana jest warstwa `"VK_LAYER_KHRONOS_validation"`, która przechwytuje błędy i niepoprawne wywołania API Vulkan.
* **Rozszerzenia (Extensions)**: Wymagane są rozszerzenia [[api/Window|GLFW]] do komunikacji z oknem systemowym, a w trybie debugowania rozszerzenie `VK_EXT_debug_utils` do logowania ostrzeżeń i błędów.

```cpp
VkInstanceCreateInfo createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
createInfo.pApplicationInfo = &appInfo;
// Dodanie warstw i rozszerzeń...
vkCreateInstance(&createInfo, nullptr, &m_instance);
```

---

## 🐛 2. Messenger Debugowania (`setupDebugMessenger`)

Aby otrzymywać szczegółowe komunikaty o błędach od warstw walidacyjnych bezpośrednio w konsoli programisty, rejestrowany jest obiekt typu `VkDebugUtilsMessengerEXT`.

* **Filtry zdarzeń**: Messenger jest skonfigurowany tak, aby nasłuchiwać komunikatów o poziomie ważności: `VERBOSE`, `INFO`, `WARNING` oraz `ERROR`.
* **Callback**: Zdarzenia są przekazywane do funkcji zwrotnej `debugCallback()`, która drukuje błędy na strumień `std::cerr`.

---

## 🖥️ 3. Powierzchnia Okna (`createSurface`)

W celu renderowania obrazu w [[api/Window|oknie GLFW]], Vulkan musi współpracować z systemem okien systemu operacyjnego (np. Windows/WGL).

Integrację tę zapewnia obiekt `VkSurfaceKHR`, tworzony przy pomocy wbudowanej funkcji GLFW:
```cpp
if (glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
}
```

---

## 🔌 4. Wybór Karty Graficznej (`pickPhysicalDevice`)

Silnik odpytuje system o dostępne GPU (`VkPhysicalDevice`) i wybiera to, które spełnia kryteria odpowiedniości w metodzie `isDeviceSuitable()`:

```cpp
bool VulkanContext::isDeviceSuitable(VkPhysicalDevice device) const {
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && 
           supportedFeatures.multiDrawIndirect; // Silnik wymaga sprzętowej obsługi MDI!
}
```

* **Wymagane funkcjonalności**: Urządzenie musi obsługiwać kolejki grafiki i prezentacji, rozszerzenie Swapchain (`VK_KHR_swapchain` opisane w [[vulkan/Krok_2_Swapchain|Krok_2_Swapchain]]) oraz posiadać sprzętową obsługę **[[vulkan/Krok_4_Zasoby_i_Synchronizacja|Multi-Draw Indirect (MDI)]]** (`supportedFeatures.multiDrawIndirect`).

---

## ⚙️ 5. Urządzenie Logiczne (`createLogicalDevice`)

Po wybraniu fizycznego procesora GPU tworzone jest logiczne urządzenie (`VkDevice`), które reprezentuje nasze połączenie z kartą graficzną.

* **Queue Families (Rodziny Kolejek)**: Konfigurowane są kolejka graficzna (Graphics Queue) do renderowania oraz kolejka prezentacji (Presentation Queue) do wyświetlania obrazu.
* **Rozszerzenia Urządzenia**: Włączane jest rozszerzenie Swapchain.
* **Włączanie Funkcji GPU i Vulkan 1.3**:
  Oprócz funkcji sprzętowych, silnik musi jawnie aktywować obsługę dynamicznego renderowania w standardzie **Vulkan 1.3**:
  ```cpp
  VkPhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.multiDrawIndirect = VK_TRUE; // Aktywowanie sprzętowego [[vulkan/Krok_4_Zasoby_i_Synchronizacja|MDI]]

  VkPhysicalDeviceVulkan13Features vulkan13Features{};
  vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  vulkan13Features.dynamicRendering = VK_TRUE; // Aktywowanie Dynamic Rendering (brak Render Passów)

  VkDeviceCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &vulkan13Features,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size()),
      .ppEnabledExtensionNames = DEVICE_EXTENSIONS.data(),
      .pEnabledFeatures = &deviceFeatures
  };
  ```
* **Biblioteka VMA (Vulkan Memory Allocator)**: Po utworzeniu logicznego urządzenia inicjalizowany jest alokator pamięci graficznej VMA (`VmaAllocator`), który służy do tworzenia i zarządzania buforami w silniku (np. [[api/MegaBuffer|MegaBuffer]]):
  ```cpp
  VmaAllocatorCreateInfo allocatorInfo{
      .physicalDevice = m_physicalDevice,
      .device = m_device,
      .instance = m_instance,
      // ...
  };
  ```
