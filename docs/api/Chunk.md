# Klasa Chunk

Klasa **`Chunk`** reprezentuje trójwymiarowy fragment świata gry o stałym rozmiarze. Odpowiada za przechowywanie voxelowych struktur danych, generowanie terenu (przy użyciu szumów szkieletowych), budowanie siatki wierzchołków przy użyciu algorytmu optymalizacyjnego [[algorithms/Greedy_Meshing|Greedy Meshing]], alokację pamięci w buforach GPU oraz zapis i odczyt stanów z dysku.

Definicja klasy znajduje się w pliku [Chunk.h](../../src/world/Chunk.h), a jej implementacja w [Chunk.cpp](../../src/world/Chunk.cpp).

---

## 🏗️ Definicja Klasy (`Chunk.h`)

```cpp
enum class BlockType : uint8_t
{
    Air   = 0,
    Grass = 1,
    Dirt  = 2,
    Stone = 3,
    Water = 6
};

struct Block
{
    BlockType type;
    uint8_t metadata;
    // ...
};

class Chunk
{
public:
    static constexpr int CHUNK_SIZE = 64;

    Chunk(glm::vec3 position, MegaBuffer* vb, MegaBuffer* ib);
    Chunk(Chunk&&)            = delete;
    Chunk& operator=(Chunk&&) = delete;
    ~Chunk();

    void generateTerrain();
    void buildMesh();

    [[nodiscard]] glm::vec3 getPosition() const { return m_position; }
    [[nodiscard]] uint32_t getIndexCount() const { return m_indexCount; }

    bool setBlock(int x, int y, int z, Block block);
    [[nodiscard]] Block getBlock(int x, int y, int z) const;

    bool save(const std::string& filepath);
    bool load(const std::string& filepath);
    void setDirty(const bool dirty) { m_isDirty = dirty; }
    void setSaveDirty(const bool dirty) { m_isSaveDirty = dirty; }
    [[nodiscard]] bool isDirty() const { return m_isDirty; }
    [[nodiscard]] bool isSaveDirty() const { return m_isSaveDirty; }

    [[nodiscard]] bool hasValidAllocation() const { return m_vertexAllocation.valid && m_indexAllocation.valid; }
    [[nodiscard]] uint32_t getVertexOffset() const { return m_vertexAllocation.offset; }
    [[nodiscard]] uint32_t getIndexOffset() const { return m_indexAllocation.offset; }

private:
    [[nodiscard]] bool isFaceVisible(int x, int y, int z) const;
    void addFace(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int x, int y, int z, int faceIndex, Block block) const;

    glm::vec3 m_position;

    MegaBuffer* m_megaVertexBuffer;
    MegaBuffer* m_megaIndexBuffer;
    BlockAllocation m_vertexAllocation;
    BlockAllocation m_indexAllocation;

    uint32_t m_indexCount = 0;

    FastNoiseLite m_noise;
    Block m_blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
    bool m_isDirty     = false;
    bool m_isSaveDirty = false;
};
```

---

## 🔑 Kluczowe Metody i Ich Rola

### 1. `void generateTerrain()`
Odpowiada za generowanie wokseli wewnątrz chunku na podstawie biblioteki **FastNoiseLite**:
* Oblicza globalne współrzędne w osiach $X$ i $Z$.
* Pobiera wartość szumu $2D$ i mapuje ją na wysokość terenu (wysokość bazowa od 15 do 75 bloków).
* Wypełnia trójwymiarową tablicę `m_blocks`: bloki na samej górze terenu jako `BlockType::Grass`, poniżej jako `BlockType::Dirt`, najgłębsze warstwy jako `BlockType::Stone`, a obszary powyżej wysokości terenu jako `BlockType::Air` (powietrze).
* Ustawia flagi `m_isDirty = true` i `m_isSaveDirty = true`.

### 2. `void buildMesh()`
Główna metoda budująca geometrię dla GPU:
1. Jeśli chunk posiadał wcześniejszą alokację w globalnych buforach, zwalnia ją wywołując `m_megaVertexBuffer->free` i `m_megaIndexBuffer->free`.
2. Uruchamia algorytm [[algorithms/Greedy_Meshing|Greedy Meshing]] w celu scalenia widocznych ścian voxelowych tego samego typu w minimalną liczbę prostokątów (quadów). Więcej w notatce **[[algorithms/Greedy_Meshing|Greedy_Meshing]]**.
3. Jeśli liczba indeksów jest większa niż 0, prosi globalny [[api/MegaBuffer|MegaBuffer]] o nową alokację o odpowiednim rozmiarze w bajtach (oddzielnie dla wierzchołków i indeksów).
4. Przesyła dane wierzchołków (`Vertex`) i indeksów (`uint32_t`) na GPU za pomocą [[api/MegaBuffer|MegaBuffer]]::upload.

### 3. `bool save(const std::string& filepath) / load(const std::string& filepath)`
Realizuje zapis i odczyt z dysku. Zamiast surowego zrzutu pamięci wokseli, implementuje algorytm [[algorithms/Kompresja_RLE|RLE (Run-Length Encoding)]], co pozwala na drastyczne zmniejszenie rozmiaru plików zapisu terenu. Zobacz **[[algorithms/Kompresja_RLE|Kompresja_RLE]]**.

---

## 🔗 Relacje z innymi klasami skarbca

* **[[api/MegaBuffer|MegaBuffer]]** — instancja chunku żąda alokacji pamięci na GPU i przesyła wierzchołki bezpośrednio do spakowanych buforów zarządzanych przez `MegaBuffer`.
* **[[api/World|World]]** — świat przechowuje zbiór chunków i wywołuje ich generowanie, ładowanie z plików oraz budowanie siatki.
* **[[api/VulkanContext|VulkanContext]]** — pobiera offsety i rozmiary wierszy z alokacji chunku w celu uzupełnienia struktur Multi-Draw Indirect (`VkDrawIndexedIndirectCommand`).
