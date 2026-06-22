# 🌲 VoxelEngine - Skarbiec Wiedzy (Obsidian Vault)

Witaj w dokumentacji **VoxelEngine**! Jest to wysokowydajny silnik voxelowy napisany w języku **C++20** z wykorzystaniem interfejsu API **Vulkan 1.3**. 

Dokumentacja ta ma na celu pomóc Ci w zrozumieniu architektury silnika, przepływu danych oraz zaawansowanych technik optymalizacji graficznej i pamięciowej, które zostały w nim zaimplementowane.

---

## 🗺️ Mapa Zawartości (Map of Content)

Skarbiec został podzielony na kilka kluczowych sekcji. Poniżej znajduje się spis głównych tematów, z którymi warto się zapoznać:

### 🎮 1. Architektura i Cykl Życia
Zrozumienie, jak uruchamia się silnik, jak zarządza pętlą główną oraz w jaki sposób obsługuje stany aplikacji i gracza.
* **[[01_Architektura_Ogolna]]** — Pętla silnika, klasa główna, cykl życia okna i zapisywanie stanów świata.

### ⚡ 2. Silnik Graficzny (Vulkan)
Opis nowoczesnego potoku graficznego opartego o Vulkan 1.3, w tym niestandardowe zarządzanie pamięcią VRAM i zaawansowane techniki rysowania.
* **[[02_Renderowanie_Vulkan]]** — Dynamic Rendering, potok graficzny, `VulkanContext`, `MegaBuffer` oraz technika Multi-Draw Indirect (MDI).

### 🌍 3. Generowanie i Zarządzanie Światem
Jak reprezentowana jest trójwymiarowa przestrzeń voxeli, jak generuje się nieskończony teren i w jaki sposób optymalizowana jest siatka geometryczna.
* **[[03_System_Swiata_i_Chunki]]** — Generowanie terenu (`FastNoiseLite`), asynchroniczny potok ładowania (`ThreadPool`), format zapisu plików oraz algorytm **Greedy Meshing**.

### ⌨️ 4. Kontrola i Interakcja
Interfejs wejściowy umożliwiający graczowi poruszanie się po świecie oraz edycję voxeli w czasie rzeczywistym.
* **[[04_System_Wejscia]]** — Obsługa GLFW, pobieranie stanów klawiatury i myszy oraz mechanika modyfikacji bloków (Raycasting i interakcja).

### 📈 5. Wydajność i Profilowanie
Informacje o technikach, dzięki którym silnik potrafi renderować miliony voxeli przy minimalnym zużyciu CPU i GPU.
* **[[05_Optymalizacje_i_Wydajnosc]]** — Face Culling, statystyki geometrii, wykresy wydajności w ImGui i korzyści z Multi-Draw Indirect.

### 📋 6. Dalszy Rozwój
Spis funkcjonalności i ulepszeń planowanych do zaimplementowania w silniku.
* **[[06_Roadmap]]** — Lista zadań (Roadmap), pomysły na fizykę, ulepszenia graficzne i rozwój świata.

---

## 🛠️ Szybki Start (Klawiszologia w Grze)
* `W, A, S, D` — Ruch kamery w płaszczyźnie poziomej.
* `Spacja / Lewy Shift` — Lot w górę / w dół.
* `TAB` — Przełączanie kursora myszy (włącza/wyłącza możliwość interakcji z debug menu ImGui).
* `Lewy Przycisk Myszy` — Niszczenie voxelów (zmiana na `BlockType::Air`).
* `Prawy Przycisk Myszy` — Stawianie voxelów (obecnie ustawia blok `BlockType::Grass`).
* `ESC` — Bezpieczne wyjście z gry (zapisuje aktualny stan świata).

---

> [!TIP]
> **Wskazówka Obsidian:** Użyj widoku grafu (`Ctrl + G` lub kliknij ikonę grafu na pasku bocznym), aby zobaczyć wizualną sieć połączeń między notatkami. Pomoże Ci to lepiej wyobrazić sobie, jak poszczególne moduły silnika wpływają na siebie nawzajem.
