# Klasa MegaBuffer

Klasa **`MegaBuffer`** odpowiada za niestandardowe zarządzanie pamięcią wierzchołków i indeksów na GPU. Zamiast alokować dziesiątki osobnych buforów Vulkan dla każdego [[api/Chunk|chunku]] świata (co wywołałoby ogromny narzut wydajnościowy sterownika), prealokuje jeden duży bufor o stałym rozmiarze i udostępnia interfejs szybkiej alokacji i zwalniania pamięci za pomocą algorytmu **First-Fit**.

Definicja klasy znajduje się w pliku [MegaBuffer.h](file:///c:/dev/repos/VoxelEngine/src/renderer/MegaBuffer.h), a jej implementacja w [MegaBuffer.cpp](file:///c:/dev/repos/VoxelEngine/src/renderer/MegaBuffer.cpp).

---

## 🏗️ Definicja Klasy (`MegaBuffer.h`)

```cpp
struct BlockAllocation
{
    uint32_t offset;
    uint32_t size;
    bool valid = false;
};

class MegaBuffer
{
public:
    MegaBuffer(VkDevice device, VmaAllocator allocator, VkDeviceSize capacity, VkBufferUsageFlags usage);
    ~MegaBuffer();

    BlockAllocation allocate(uint32_t size);
    void free(const BlockAllocation& block);

    void upload(const BlockAllocation& block, const void* data) const;

    [[nodiscard]] VkBuffer getBuffer() const { return m_buffer; }

private:
    VmaAllocator m_allocator;
    VkBuffer m_buffer          = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    void* m_mappedData         = nullptr;

    struct FreeBlock
    {
        uint32_t offset;
        uint32_t size;
    };

    std::list<FreeBlock> m_freeBlocks;
    std::mutex m_mutex;
};
```

---

## 🔑 Kluczowe Metody i Ich Rola

### 1. Inicjalizacja (`MegaBuffer::MegaBuffer`)
W konstruktorze prealokowany jest jeden duży bufor o rozmiarze `capacity` (w bajtach) przy użyciu biblioteki VMA. Zastosowanie flagi `VMA_ALLOCATION_CREATE_MAPPED_BIT` powoduje, że bufor jest **stale zmapowany** (CPU posiada stały wskaźnik `m_mappedData` do pamięci GPU), eliminując kosztowne wywołania systemowe mapowania pamięci:
```cpp
VmaAllocationCreateInfo allocInfo{};
allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
```
Początkowo cała pula pamięci jest oznaczona jako jeden wolny blok w liście dwukierunkowej `m_freeBlocks`.

### 2. `BlockAllocation allocate(const uint32_t size)`
Alokuje blok pamięci o rozmiarze `size` (w bajtach) wewnątrz mega-bufora przy użyciu algorytmu **First-Fit**:
* Przebiega po liście wolnych bloków `m_freeBlocks`.
* Gdy znajdzie pierwszy blok o rozmiarze większym lub równym `size`, dzieli go:
  * Zwraca strukturę `BlockAllocation` z offsetem przydzielonej pamięci.
  * Pomniejsza wolny blok o zaalokowany rozmiar lub całkowicie usuwa go z listy, jeśli rozmiary były równe.
* Operacja jest chroniona mutexem `m_mutex` w celu zapewnienia bezpieczeństwa wątkowego (generowanie siatki [[api/Chunk|chunków]] zachodzi na wielu wątkach).

### 3. `void free(const BlockAllocation& block)`
Zwalnia wcześniej zaalokowany blok pamięci:
1. Dodaje zwolniony blok z powrotem do listy `m_freeBlocks`.
2. **Sortowanie**: Sortuje listę wolnych bloków rosnąco według ich przesunięć (offsetów).
3. **Koalescencja (Scalanie)**: Przebiega liniowo po posortowanej liście i scala sąsiadujące ze sobą wolne bloki w jeden większy, co zapobiega fragmentacji pamięci GPU:
   ```cpp
   if (it->offset + it->size == next->offset) {
       it->size += next->size;
       m_freeBlocks.erase(next);
   }
   ```

### 4. `void upload(const BlockAllocation& block, const void* data) const`
Przesyła dane geometryczne z pamięci operacyjnej CPU do pamięci karty graficznej. Ponieważ bufor jest stale zmapowany, operacja ta sprowadza się do szybkiej operacji kopiowania pamięci RAM przy użyciu `std::memcpy` z odpowiednim przesunięciem alokacji:
```cpp
std::memcpy(static_cast<char*>(m_mappedData) + block.offset, data, block.size);
```

---

## 🔗 Relacje z innymi klasami skarbca

* **[[api/VulkanContext|VulkanContext]]** — kontekst tworzy i przechowuje dwie instancje mega-buforów: `m_megaVertexBuffer` (512 MB) oraz `m_megaIndexBuffer` (256 MB).
* **[[api/Chunk|Chunk]]** — podczas generowania siatki wokseli, każdy [[api/Chunk|chunk]] alokuje w tych buforach wierzchołki i indeksy oraz przesyła je za pomocą metody `upload()`.
* **[[algorithms/Greedy_Meshing|Greedy_Meshing]]** — algorytm przygotowuje dane, które są wysyłane do [[api/MegaBuffer|MegaBuffer]].
