# Klasa World

Klasa **`World`** reprezentuje strukturę świata wokselowego składającego się z trójwymiarowej siatki [[api/Chunk|chunków]]. Odpowiada za asynchroniczne zarządzanie cyklem życia [[api/Chunk|chunków]] wokół [[api/Camera|gracza]] (ładowanie, generowanie terenu, budowanie siatek, usuwanie z pamięci), zapis i odczyt stanów na dysk oraz przeprowadzanie interakcji [[api/Camera|gracza]] z voxelami za pomocą [[algorithms/Raycasting_DDA|raycastingu DDA]].

Definicja klasy znajduje się w pliku [World.h](file:///c:/dev/repos/VoxelEngine/src/world/World.h), a jej implementacja w [World.cpp](file:///c:/dev/repos/VoxelEngine/src/world/World.cpp).

---

## 🏗️ Definicja Klasy (`World.h`)

```cpp
class World
{
public:
    World() = delete;
    explicit World(VulkanContext* context);
    ~World();

    void update(const glm::vec3& cameraPos, const glm::vec3& cameraFront, float deltaTime);
    void changeWorld(const std::string& newPath);
    [[nodiscard]] const std::string& getWorldPath() const { return m_worldPath; }

    [[nodiscard]] int getRenderDistance() const { return m_renderDistance; }
    void setRenderDistance(const int distance) { m_renderDistance = distance; }

    void saveAllChunks() const;

    void processPlayerInteraction(const glm::vec3& cameraPos, const glm::vec3& cameraFront, bool leftClick, bool rightClick);
    [[nodiscard]] Block getBlockAt(int x, int y, int z) const;
    void setBlockAt(int x, int y, int z, Block block);

    [[nodiscard]] const std::vector<Chunk*>& getActiveChunks() const { return m_activeChunks; }

private:
    VulkanContext* m_vulkanContext;

    struct ChunkCoord {
        int x, y, z;
        bool operator==(const ChunkCoord& other) const { return x == other.x && y == other.y && z == other.z; }
    };

    struct ChunkCoordHash {
        std::size_t operator()(const ChunkCoord& k) const {
            return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 1) ^ (std::hash<int>()(k.z) << 2);
        }
    };

    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> m_chunkMap;
    std::vector<Chunk*> m_activeChunks;

    int m_renderDistance    = 8;
    float m_updateTimer     = 0.0f;
    std::string m_worldPath = "saves/world1/";

    struct ChunkGarbage {
        std::unique_ptr<Chunk> chunk;
        uint64_t frameRemoved;
    };

    std::vector<ChunkGarbage> m_chunksToDelete;
    std::mutex m_deletionMutex;
    uint64_t m_frameCount = 0;

    std::unordered_map<ChunkCoord, std::future<std::unique_ptr<Chunk>>, ChunkCoordHash> m_chunkFutures;
    std::unique_ptr<ThreadPool> m_threadPool;
};
```

---

## 🔑 Kluczowe Metody i Ich Rola

### 1. `void update(const glm::vec3& cameraPos, const glm::vec3& cameraFront, float deltaTime)`
Wywoływana w każdej klatce. Wykonuje następujące zadania:
* **Asynchroniczne ładowanie**: Jeśli timer aktualizacji przekroczy próg (np. co 0.2 sekundy), oblicza pozycję gracza w koordynatach chunków i skanuje obszar w promieniu `m_renderDistance`.
* Brakujące chunki są asynchronicznie przydzielane do puli wątków roboczych w celu załadowania z pliku lub wygenerowania terenu i budowy siatki.
* **Czyszczenie chunków**: Zwalnia pamięć i zapisuje chunki, które wyszły poza zasięg widoczności gracza. Używa mutexu `m_deletionMutex` do wątkowo bezpiecznego dodawania ich do listy do usunięcia.
* **Sprawdzanie przyszłości (`m_chunkFutures`)**: Sprawdza, czy wątki ukończyły generowanie chunków. Gotowe chunki są dodawane do `m_chunkMap`.
* **Kolekcjonowanie aktywnych chunków**: Tworzy wektor `m_activeChunks` z chunków, które mają poprawne alokacje VRAM na GPU. Sortuje je **od najbliższego do najdalszego** (Front-to-Back) w celu optymalizacji Early-Z.

### 2. `void changeWorld(const std::string& newPath)`
Zamyka bieżący świat, czyści pulę zadań w `ThreadPool`, czeka na bezczynność GPU, zapisuje wszystkie zmodyfikowane chunki na dysk i ładuje ścieżkę do nowego świata.

### 3. `void processPlayerInteraction(...)`
Uruchamia algorytm **DDA Raycasting** z pozycji [[api/Camera|kamery gracza]]. Wykrywa trafiony blok, a następnie:
* Jeśli kliknięto lewy przycisk: niszczy go (`BlockType::Air`).
* Jeśli kliknięto prawy przycisk: stawia nowy blok `BlockType::Stone` po stronie uderzenia normalnej.
Więcej szczegółów w notatce **[[algorithms/Raycasting_DDA|Raycasting DDA]]**.

### 4. `Block getBlockAt(int x, int y, int z) const`
Zwraca strukturę bloku na globalnych współrzędnych świata. Wylicza, w którym [[api/Chunk|chunku]] znajduje się punkt, przekształca współrzędne globalne na lokalne współrzędne chunku $[0, 63]$ i odpytuje konkretną instancję klasy [[api/Chunk|Chunk]].

---

## 🔗 Relacje z innymi klasami skarbca

* **[[api/Chunk|Chunk]]** — klasa `World` zarządza kontenerem [[api/Chunk|chunków]], inicjalizuje je i wywołuje ich metody.
* **[[api/VulkanContext|VulkanContext]]** — referencja przekazywana do [[api/Chunk|chunków]] podczas ich tworzenia w celu uzyskania dostępu do [[api/MegaBuffer|MegaBuffer]] w celu alokacji wierzchołków i indeksów na GPU.
* **[[algorithms/Raycasting_DDA|Raycasting DDA]]** — algorytm interakcji gracza z voxelami w świecie.
* **[[algorithms/Kompresja_RLE|Kompresja RLE]]** — serializacja i deserializacja bloków podczas zapisywania i wczytywania plików binarnych.
