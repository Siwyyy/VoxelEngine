# 📝 Roadmap

Oto lista potencjalnych funkcjonalności, ulepszeń i optymalizacji, które można zaimplementować w silniku **VoxelEngine** w ramach dalszego rozwoju.

---

## 🎮 1. Fizyka i Poruszanie się
Urealnienie zachowania gracza w świecie voxelowym.
- [ ] **Grawitacja i spadanie**: Dodanie stałego przyspieszenia w dół działającego na gracza, gdy nie jest w trybie niszczenia/budowania.
- [ ] **Kolizje z voxelami**: Wykrywanie kolizji typu AABB (pudełko kolizyjne gracza z blokami) uniemożliwiające przechodzenie przez ściany i ziemię.
- [ ] **Chodzenie i skakanie**: Dodanie obsługi spacji do skoku, gdy gracz dotyka ziemi, oraz ograniczenie lotu kamery (klawisz przełączający tryb latania Spectator vs tryb chodzenia).
- [ ] **Ślizganie i tarcie**: Różne zachowanie w zależności od podłoża (np. lód).

---

## 🎨 2. Ulepszenia Graficzne i Wizualne
Poprawa estetyki renderowania voxelowego świata.
- [ ] **Wokselowe Ambient Occlusion (AO)**: Cieniowanie styków i narożników bloków (przyciemnianie wierzchołków na podstawie obecności sąsiednich voxeli podczas budowania siatki). Nadaje silnikowi głębię 3D.
- [ ] **Dynamiczne oświetlenie**: W pełni dynamiczne źródła światła (np. punktowe/reflektorowe światło przy pochodniach lub graczu) o płynnym zanikaniu intensywności, niezależne od siatki bloków.
- [ ] **Cykl dnia i nocy**: Dynamiczna zmiana koloru tła nieba (clear color) oraz kierunku i barwy światła słonecznego na podstawie czasu systemowego.

---

## 🛠️ 3. Rozgrywka i Edycja Świata
Wzbogacenie interakcji ze światem gry.
- [ ] **Wybór bloków (Hotbar / Pasek wyboru)**:
  - Dodanie paska wyboru bloków w ImGui.
  - Wybór aktywnego bloku za pomocą cyfr `1-4` na klawiaturze.
- [ ] **Dynamiczna fizyka płynów**: W pełni dynamiczny algorytm symulujący naturalne przelewanie i zachowanie płynów (np. modelowanie objętościowe oparte na automatach komórkowych lub uproszczona fizyka cząsteczkowa), pozwalający na realistyczny przepływ wody i wyrównywanie poziomów.
- [ ] **Generator struktur (Drzewa, jaskinie)**:
  - Generowanie jaskiń przy użyciu szumu 3D Perlina/Simplex.
  - Dodawanie prostych struktur, takich jak drzewa (pień z drewna, liście) podczas generowania chunków.

---

## ⚙️ 4. Systemy Podsilnikowe i Optymalizacje
Usprawnienia pod maską.
- [ ] **Zapis ustawień do pliku**: Zapisywanie render distance, czułości myszy i innych opcji do pliku `config.json` lub `.ini`.
- [ ] **Occlusion Culling (GPU)**: Odrzucanie całych chunków, które są całkowicie zasłonięte przez inne chunki znajdujące się bliżej kamery (np. za pomocą Hi-Z lub zapytań o zajętość pikseli).

---

## 🚀 5. Modernizacja Kodu (C++20 / C++23)
Stopniowe wprowadzanie nowoczesnych standardów języka w celu poprawy bezpieczeństwa, czytelności i wydajności.

### 📦 Core, Input i Utils
- [ ] **`std::expected` zamiast wyjątków w `FileSystem::readFile`**: Bezpieczniejsza, funkcyjna obsługa błędów odczytu plików bez narzutu wyjątków.
- [ ] **Algorytmy `std::ranges` w statystykach i culling'u**: Użycie `std::ranges::fold_left` w `VoxelEngine.cpp` do liczenia średniego czasu klatki oraz `std::ranges::all_of` w `Frustum.cpp` zamiast klasycznych pętli.
- [x] **Mocno typowane klawisze (`std::to_underlying`)**: Zastąpienie makr `LAVA_KEY_...` w `KeyCodes.h` oraz `LAVA_MOUSE_...` w `MouseButtonCodes.h` przez `enum class KeyCode : int32_t` i `enum class MouseButton : int32_t`, z rzutowaniem przez `std::to_underlying`.
- [ ] **Wątki z `std::move_only_function`**: Zastąpienie `std::function` w kolejce `ThreadPool::m_tasks` na rzecz `std::move_only_function`, co eliminuje potrzebę alokacji przez `std::make_shared` w `enqueue()`.

### 🌍 World (Świat)
- [ ] **Płaskie tablice z widokiem `std::mdspan`**: Zastąpienie zagnieżdżonych tablic C-style w klasie `Chunk` (`Block m_blocks[32][32][32]`) spłaszczonym `std::array<Block, CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE>` z widokiem `std::mdspan` i wielowymiarowym operatorem `[]` z C++23.
- [ ] **Eliminacja zagnieżdżonych pętli (`std::views::cartesian_product`)**: Skrócenie generowania chunków w `Chunk::generateTerrain()` przy użyciu potoków ranges.
- [x] **Formatowanie ścieżek i logów (`std::format` / `std::print`)**: Zastąpienie wolnego `std::stringstream` oraz klasycznego `std::cout`/`std::cerr` przez `std::format` i C++23-standardowe `std::print`/`std::println`.

### 🎨 Renderer & Vulkan
- [ ] **Zastąpienie par wskaźnik + rozmiar przez `std::span`**: Zabezpieczenie operacji kopiowania buforów w `MegaBuffer` poprzez templatyzowane `upload` przyjmujące `std::span<const T>`.
- [ ] **Potoki `std::ranges` w generowaniu komend rysowania**: Zastąpienie tradycyjnej pętli generującej komendy pośrednie rysowania (`VkDrawIndexedIndirectCommand`) w `VulkanContext::drawFrame` potokiem z `std::views::filter` i `std::views::transform`, kopiującym dane bezpośrednio do zmapowanej pamięci za pomocą `std::ranges::copy`.
- [ ] **Monadyczny `std::optional` w alokacji**: Zmiana typu zwracanego przez `MegaBuffer::allocate` na `std::optional<BlockAllocation>` i wykorzystanie operacji łańcuchowych takich jak `and_then` lub `or_else`.
- [x] **Optymalizacja `std::vector` na `std::array` w Vertex**: Zmiana typu zwracanego z `Vertex::getAttributeDescriptions()` na `std::array<VkVertexInputAttributeDescription, 2>`, eliminująca dynamiczną alokację wektora.
