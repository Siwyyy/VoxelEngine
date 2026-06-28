# Krok 3 - Dynamic Rendering i Potok Graficzny

W tym kroku [[api/VoxelEngine|silnik graficzny]] tworzy [[api/GraphicsPipeline|potok graficzny (GraphicsPipeline)]] obsługujący renderowanie dynamiczne (Dynamic Rendering), rezygnując z tradycyjnych Render Passów na rzecz elastyczności wprowadzonej w Vulkanie 1.3.

Kod ten znajduje się w pliku [GraphicsPipeline.cpp](../../src/renderer/GraphicsPipeline.cpp) (klasy [[api/GraphicsPipeline|GraphicsPipeline]]).

---

## 🎨 1. Dynamic Rendering w Potoku

Zamiast przekazywać wskaźnik do obiektu `VkRenderPass` podczas tworzenia [[api/GraphicsPipeline|potoku graficznego]], silnik używa struktury **`VkPipelineRenderingCreateInfo`** podpiętej przez pole `pNext` w głównej strukturze inicjalizacyjnej.

Ustawiane są tam formaty załączników koloru (pochodzącego ze [[vulkan/Krok_2_Swapchain|Swapchaina]]) oraz głębokości:
```cpp
VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &swapchainImageFormat,
    .depthAttachmentFormat = depthFormat
};
```

Dzięki temu pole `renderPass` w strukturze `VkGraphicsPipelineCreateInfo` może pozostać pustym uchwytem (`VK_NULL_HANDLE`):
```cpp
VkGraphicsPipelineCreateInfo pipelineInfo{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = &pipelineRenderingCreateInfo,
    .layout = m_layout,
    .renderPass = VK_NULL_HANDLE, // Zamiast tradycyjnego Render Passu
    // ...
};
```

---

## ⚙️ 2. Konfiguracja Stanów Potoku (Pipeline States)

Metoda [[api/GraphicsPipeline|GraphicsPipeline::GraphicsPipeline()]] precyzyjnie definiuje stany operacji wykonywanych na GPU:

* **Vertex Input**: Wiąże [[api/Chunk|strukturę wierzchołka Vertex]] (pozycja i kolor) z wejściem shadera.
* **Input Assembly**: Konfiguruje topologię jako listę trójkątów (`VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST`).
* **Rasterizer**:
  * `polygonMode`: Ustawiony na `VK_POLYGON_MODE_FILL` (wypełnianie brył).
  * `cullMode`: Ustawiony na **`VK_CULL_MODE_BACK_BIT`** (włącza culling tylnych ścian sześcianów, co oszczędza moc obliczeniową GPU).
  * `frontFace`: Ustawiony na **`VK_FRONT_FACE_COUNTER_CLOCKWISE`** (ściany zorientowane przeciwnie do ruchu wskazówek zegara uznawane są za przednie).
* **Depth Stencil**: Włącza test głębokości (`depthTestEnable = VK_TRUE`) z funkcją porównania `VK_COMPARE_OP_LESS` – piksele bliższe kamerze nadpisują te dalsze (szerzej o buforze głębokości w [[vulkan/Krok_2_Swapchain|Krok 2 - Swapchain]]).

---

## ⚡ 3. Dynamiczne Stany (Dynamic States) & Push Constants

Silnik definiuje stany **Viewport** (obszar rysowania) oraz **Scissor** (obszar przycinania) jako stany dynamiczne (`VK_DYNAMIC_STATE_VIEWPORT`, `SCISSOR`). Pozwala to na zmianę rozdzielczości [[api/Window|okna gry]] w trakcie działania programu bez konieczności ponownego tworzenia całego [[api/GraphicsPipeline|potoku graficznego]].

### Push Constants (Stałe szybkiego dostępu)
Przekazywanie macierzy transformacji $MVP$ (Model-View-Projection) obliczanej przez [[api/Camera|Camera]] za pomocą Descriptor Setów wiązałoby się z alokacją pamięci i narzutem CPU. Silnik wykorzystuje **Push Constants** – mały, bardzo szybki obszar pamięci na GPU (zazwyczaj 128 lub 256 bajtów), do którego dane wierzchołków są kopiowane bezpośrednio z CPU podczas rysowania.

Silnik rezerwuje w potoku przestrzeń o rozmiarze macierzy $4\times4$:
```cpp
VkPushConstantRange pushConstantRange{
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .offset     = 0,
    .size       = sizeof(glm::mat4) // 64 bajty na macierz MVP
};
```

---

## 🔮 4. Shaders Module

Konstruktor klasy [[api/GraphicsPipeline|GraphicsPipeline]] przyjmuje dwa skompilowane shadery (`Shader`), reprezentowane na GPU jako obiekty `VkShaderModule`:
1. **Shader Wierzchołków (`shaders/shader.vert`)**: Odpowiada za transformację pozycji wierzchołków z przestrzeni świata do przestrzeni ekranu za pomocą macierzy MVP przekazanej przez Push Constants (zobacz [[api/Camera|Camera]]).
2. **Shader Fragmentów (`shaders/shader.frag`)**: Oblicza ostateczny kolor pikseli bloków, nakładając proste cieniowanie kierunkowe na podstawie normalnej ściany voxela.
