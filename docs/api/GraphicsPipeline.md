# Klasa GraphicsPipeline

Klasa **`GraphicsPipeline`** odpowiada za konfigurację oraz tworzenie potoku graficznego (Graphics Pipeline Layout i Pipeline State Object) w interfejsie Vulkan. Integruje ładowanie shaderów SPIR-V, układ danych wierzchołkowych, konfigurację stanów stałych karty graficznej (rasteryzacja, test głębokości) oraz definiuje zakresy dynamicznego przesyłania stałych (Push Constants).

Definicja klasy znajduje się w pliku [GraphicsPipeline.h](../../src/renderer/GraphicsPipeline.h), a jej implementacja w [GraphicsPipeline.cpp](../../src/renderer/GraphicsPipeline.cpp).

---

## 🏗️ Definicja Klasy (`GraphicsPipeline.h`)

```cpp
namespace voxl
{
    class Shader;

    class GraphicsPipeline
    {
    public:
        GraphicsPipeline(VkDevice device, VkFormat swapchainImageFormat, VkFormat depthFormat, const Shader& vertShader,
                         const Shader& fragShader);
        ~GraphicsPipeline();

        GraphicsPipeline(const GraphicsPipeline&)            = delete;
        GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

        [[nodiscard]] VkPipeline getPipeline() const { return m_pipeline; }
        [[nodiscard]] VkPipelineLayout getLayout() const { return m_layout; }

        void bind(VkCommandBuffer commandBuffer) const;

    private:
        VkDevice m_device;
        VkPipeline m_pipeline     = VK_NULL_HANDLE;
        VkPipelineLayout m_layout = VK_NULL_HANDLE;
    };
}
```

---

## 🔑 Kluczowe Metody i Ich Rola

### 1. `GraphicsPipeline(...)` (Konstruktor)
Wykonuje całą procedurę konfiguracji i tworzenia potoku graficznego dla GPU:
* **Shader Stages**: Określa dwa etapy cieniowania na podstawie przekazanych obiektów `Shader` (Vertex Shader oraz Fragment Shader).
* **Vertex Input State**: Pobiera opis wiązania wierzchołków (`Vertex::getBindingDescription` i `Vertex::getAttributeDescriptions` zdefiniowanych w [[api/Chunk|Vertex.h]]) w celu zdefiniowania formatu przesyłanych współrzędnych pozycji (3x float) oraz koloru (3x float) do shadera wierzchołków.
* **Pipeline Layout**: Tworzy układ potoku (`m_layout`) z włączonym jednym zakresem Push Constants (`VkPushConstantRange`) o rozmiarze `sizeof(glm::mat4)` dla przesyłania macierzy MVP bezpośrednio z procesora do shadera wierzchołków.
* **Dynamic Rendering (`VkPipelineRenderingCreateInfo`)**: Podłącza formaty [[vulkan/Krok_3_Potok_Graficzny|renderowania dynamicznego]] (format obrazu swapchaina i format głębokości) przez pole `pNext` w głównej strukturze `VkGraphicsPipelineCreateInfo`.
* **Tworzenie potoku**: Inicjalizuje obiekt `VkPipeline` za pomocą systemowej funkcji `vkCreateGraphicsPipelines()`.

### 2. `~GraphicsPipeline()` (Destruktor)
Zwalnia zasoby potoku graficznego na GPU, wywołując funkcje `vkDestroyPipeline` oraz `vkDestroyPipelineLayout`.

### 3. `void bind(const VkCommandBuffer commandBuffer) const`
Wiązanie potoku z buforem poleceń. Informuje GPU, że kolejne polecenia rysowania mają być przetwarzane zgodnie z regułami zdefiniowanymi w tym obiekcie:
```cpp
void GraphicsPipeline::bind(const VkCommandBuffer commandBuffer) const
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}
```

---

## 🔗 Relacje z innymi klasami skarbca

* **[[Shader]]** — potok wymaga dwóch instancji shaderów w celu zdefiniowania etapów wykonywania kodu programu na GPU.
* **[[api/VulkanContext|VulkanContext]]** — kontekst tworzy potok graficzny podczas inicjalizacji i wywołuje metodę `bind()` przed renderowaniem geometrii [[api/Chunk|chunków]].
* **[[vulkan/Krok_3_Potok_Graficzny|Krok_3_Potok_Graficzny]]** — notatka szczegółowo opisująca każdy krok konfiguracji potoku graficznego.
