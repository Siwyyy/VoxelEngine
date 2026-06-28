# Klasa VoxelEngine

Klasa **`VoxelEngine`** jest główną klasą kontrolną całego silnika. Odpowiada za orkiestrację cyklu życia gry, w tym inicjalizację [[api/Window|okna]], [[api/Input|wejścia]], [[api/Camera|kamery]], [[api/World|systemu świata]], zarządzanie pętlą główną (Game Loop) z synchronizacją czasu klatki za pomocą klasy [[api/Time|Time]], oraz wyświetlanie interfejsu diagnostycznego za pomocą Dear ImGui.

Definicja klasy znajduje się w pliku [VoxelEngine.h](../../src/core/VoxelEngine.h), a jej implementacja w [VoxelEngine.cpp](../../src/core/VoxelEngine.cpp).

---

## 🏗️ Definicja Klasy (`VoxelEngine.h`)

```cpp
class VoxelEngine
{
public:
    VoxelEngine();
    ~VoxelEngine();

    void run();

private:
    void switchWorld(const std::string& worldName) const;
    void savePlayerState() const;
    void loadPlayerState(const std::string& worldName) const;

    std::unique_ptr<Window> m_window;
    std::unique_ptr<class Input> m_input;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<World> m_world;
    VulkanContext m_vulkanContext;

    float m_clickCooldown      = 0.0f;
    bool m_cursorEnabled       = false;
    bool m_tabPressedLastFrame = false;
    std::string m_pendingWorldLoad;

    float m_worldSizeUpdateTimer = 1.0f;
    uintmax_t m_currentWorldSize = 0;
    std::future<uintmax_t> m_worldSizeFuture;
    std::future<void> m_backgroundTask;

    std::vector<float> m_frameTimes;
    std::vector<float> m_gpuFrameTimes;
    size_t m_frameTimeIndex  = 0;
    float m_graphUpdateTimer = 0.0f;

    const uint32_t m_width  = 1600;
    const uint32_t m_height = 900;

    void initVulkan();
    void mainLoop();
    void cleanup();
};
```

---

## 🔑 Kluczowe Metody i Ich Rola

### 1. `void run()`
Główny punkt startowy silnika wywoływany w funkcji `main()`. Inicjalizuje punkty czasowe za pomocą `[[api/Time|Time]]::init()`, uruchamia sekwencyjnie inicjalizację Vulkan (`initVulkan`), pętlę klatki (`mainLoop`), a po zakończeniu sprząta zasoby (`cleanup`).

### 2. `void mainLoop()`
Pętla gry działająca w każdej klatce. Do jej zadań należy:
* Odświeżanie czasu gry i obliczanie Delta Time za pomocą `[[api/Time|Time]]::tick()` na początku każdej klatki.
* Aktualizowanie tablic wydajności (`m_frameTimes`, `m_gpuFrameTimes`) z czasem Delta Time dostarczanym przez klasę [[api/Time|Time]].
* Obsługa żądania przeładowania świata (`switchWorld`) oraz asynchronicznego pomiaru wagi zapisu na dysku (`m_worldSizeFuture`).
* Zbieranie zdarzeń z [[api/Window|okna]] (`Window::pollEvents`).
* Wykrywanie klawiszy funkcyjnych (np. `ESC` wyjście, `TAB` zwalnianie kursora myszy).
* Aktualizacja [[api/Camera|kamery]] (`Camera::update`) i [[api/World|świata]] (`World::update`) pod warunkiem przechwyconego kursora, z wykorzystaniem `Time::getDeltaTime()`.
* Rysowanie interfejsu debugowania Dear ImGui oraz wywołanie renderowania sceny za pomocą `[[api/VulkanContext|VulkanContext]]::drawFrame`.

### 3. `void switchWorld(const std::string& worldName)`
Zmienia aktualnie aktywny świat:
1. Zapisuje dotychczasowy stan pozycji i obrotu gracza.
2. Zmienia ścieżkę w `[[api/World|World]]` i czyści stare [[api/Chunk|chunki]].
3. Wczytuje pozycję gracza z nowego zapisu `player.dat`.

### 4. `void savePlayerState() / loadPlayerState()`
Odpowiada za binarną serializację pozycji [[api/Camera|kamery]] gracza (`glm::vec3`) oraz jej kątów obrotu (`m_yaw`, `m_pitch`) do pliku `player.dat`.

---

## 🔗 Relacje z innymi klasami skarbca

* **[[api/Window|Window]]** — silnik tworzy i pobiera uchwyt GLFW z okna w celu sterowania i przekazania do kontekstu Vulkan.
* **[[api/Input|Input]]** — inicjalizuje platformową implementację wejścia (`GLFWInput`).
* **[[api/Camera|Camera]]** — przechowuje instancję kamery jako reprezentację gracza w świecie.
* **[[api/World|World]]** — zarządza stanem bloków wokselowych, wątkami oraz raycastingiem.
* **[[api/VulkanContext|VulkanContext]]** — zarządza potokiem renderowania Vulkan.
* **[[api/Time|Time]]** — silnik orkiestruje wywołania tick na początku klatki i odpytuje klasę o aktualne czasy klatek.
