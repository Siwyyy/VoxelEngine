# 📈 Optymalizacje i Wydajność

Silnik [[api/VoxelEngine|VoxelEngine]] został zaprojektowany z myślą o wysokiej wydajności (target: 60+ FPS przy bardzo dużym zasięgu renderowania). Osiągnięto to dzięki wdrożeniu zestawu zaawansowanych technik optymalizacji graficznej, pamięciowej i procesorowej.

Poniżej opisano najważniejsze z nich.

---

## 🪓 1. Optymalizacja Geometrii (Siatki)

Klasyczne renderowanie wokseli polega na rysowaniu każdego sześcianu jako osobnego modelu z 12 trójkątami. Przy rozmiarze [[api/Chunk|chunku]] $64 \times 64 \times 64$ daje to $262\ 144$ sześcianów, co przy naiwnym podejściu generowałoby ponad **3.1 miliona trójkątów** na jeden [[api/Chunk|chunk]].

Silnik redukuje to za pomocą:
* **Face Culling (Kompaktowanie ścian)**: Ściany graniczące z innymi pełnymi blokami są usuwane. Renderowane są tylko ściany stykające się z powietrzem (`BlockType::Air` zdefiniowanym w [[api/Chunk|Chunk.h]]).
* **[[algorithms/Greedy_Meshing|Greedy Meshing]]**: Algorytm łączy sąsiadujące, identyczne kwadratowe ściany w jeden duży prostokąt (quad). Zamiast np. 64 osobnych quadaów dla ziemi, renderowany jest tylko 1.
  * **Rezultat**: Redukcja liczby wierzchołków i indeksów o **ponad 90%** (zobacz szczegóły w **[[03_System_Swiata_i_Chunki]]**).

---

## 👁️ 2. Eliminacja Niewidocznych Obiektów ([[algorithms/Frustum_Culling|Frustum Culling]])

Przed wysłaniem obiektów do renderowania na GPU, silnik sprawdza, czy znajdują się one w polu widzenia kamery (Frustum).

1. **Ekstrakcja Płaszczyzn**: Na podstawie macierzy widoku-projekcji (VP Matrix) w klasie [[api/Frustum|Frustum]] wyodrębniane jest 6 płaszczyzn definiujących ostrosłup ścięty widoku kamery.
2. **Test AABB (Axis-Aligned Bounding Box)**: Każdy [[api/Chunk|chunk]] posiada zdefiniowane pudełko otaczające o rozmiarach $64 \times 64 \times 64$ (przeskalowane przez rozmiar bloku).
3. **Selekcja**:
   ```cpp
   if (!frustum.intersectsAABB(minAABB, maxAABB)) {
       continue; // Pomiń ten chunk - nie jest przesyłany do renderowania
   }
   ```
   Dzięki temu układ graficzny nie przetwarza [[api/Chunk|chunków]] znajdujących się za plecami gracza ani daleko z boku.

---

## 🚀 3. Multi-Draw Indirect (MDI)

W klasycznym renderingu CPU musi wywołać osobną komendę rysowania (`DrawIndexed`) dla każdego [[api/Chunk|chunku]] osobno, co generuje ogromny narzut na sterownik graficzny (driver overhead) i ogranicza wydajność gry do procesora (CPU bottleneck).

Zastosowanie **[[vulkan/Krok_4_Zasoby_i_Synchronizacja|Multi-Draw Indirect]]** pozwala na:
* Spakowanie wszystkich komend rysowania w jedną tablicę w pamięci GPU (`m_indirectBuffer` zdefiniowanym w [[api/VulkanContext|VulkanContext]]).
* Wywołanie rysowania całego świata gry za pomocą **jednego polecenia rysującego** na CPU (`vkCmdDrawIndexedIndirect`).
* Rysowanie na GPU odbywa się równolegle, z pominięciem interwencji procesora w trakcie renderowania poszczególnych [[api/Chunk|chunków]].

---

## 💾 4. Globalna Pamięć GPU ([[api/MegaBuffer|MegaBuffer]])

Zamiast tworzyć i usuwać bufory wierzchołków przy każdej modyfikacji terenu przez gracza (co powodowałoby drastyczny spadek FPS z powodu alokacji pamięci na GPU), silnik rezerwuje z góry stałe, duże obszary VRAM ([[api/MegaBuffer|MegaBuffer]]):
* Jeden wielki bufor Vertex Buffer na wszystkie wierzchołki świata.
* Jeden wielki bufor Index Buffer na wszystkie indeksy.
* **Custom Allocator**: Niestandardowy, szybki menedżer pamięci wewnątrz silnika rozdziela fragmenty buforów dla poszczególnych [[api/Chunk|chunków]] bez narzutu systemowego Vulkan.

---

## 🧵 5. Praca Wielowątkowa (Asynchroniczny Teren)

Wszelkie operacje wejścia-wyjścia (zapis/odczyt z dysku) oraz operacje CPU-intensive (generowanie szumu terenu, [[algorithms/Greedy_Meshing|greedy meshing]]) są delegowane do **puli wątków roboczych** (`ThreadPool` zdefiniowanej w [[api/World|World.h]]):
* Główny wątek renderowania nie czeka na wygenerowanie nowego terenu.
* [[api/Chunk|Chunk]] pojawia się na ekranie płynnie dopiero po pełnym wygenerowaniu i przesłaniu danych do [[api/MegaBuffer|MegaBuffer]] na wątku pobocznym.
* **Early-Z Sorting**: Aktywne [[api/Chunk|chunki]] są sortowane od najbliższego do najdalszego względem gracza. Karta graficzna dzięki temu najpierw rysuje obiekty blisko kamery, a te położone dalej są odrzucane na etapie testu głębokości (Depth Test), co zapobiega zjawisku *overdraw* (wielokrotnego rysowania tych samych pikseli).

---

## 📊 6. Profilowanie Wydajności w GUI

Silnik posiada wbudowane narzędzia diagnostyczne wyświetlane za pomocą biblioteki **Dear ImGui**:

```
┌────────────────────────────────────────────────────────┐
│ Debug Info                                             │
├────────────────────────────────────────────────────────┤
│ World: world1 (12.45 MB)                               │
│ FPS: 124.5                                             │
│ Active Chunks: 120    Drawn Chunks: 84                 │
│ Total Quads: 312,450                                   │
│ Total Vertices: 1,249,800                              │
│ Total Indices: 1,874,700                               │
└────────────────────────────────────────────────────────┘
```

* **GPU Frame Time ([[vulkan/Krok_4_Zasoby_i_Synchronizacja|VkQueryPool]])**: Silnik pobiera dokładny czas renderowania klatki na karcie graficznej za pomocą timestampów Vulkan. Zapobiega to zakłamywaniu wyników pomiaru czasu przez czas uśpienia wątku CPU (vsync/fences).
* **CPU vs GPU Graph**: Narzędzie rysuje w czasie rzeczywistym wykres porównujący czas pracy procesora i karty graficznej na klatkę w milisekundach, ułatwiając diagnostykę wąskich gardeł (bottlenecks).
* **Asynchroniczny licznik rozmiaru świata**: Rozmiar plików zapisu świata na dysku jest skanowany asynchronicznie w tle (`std::future` i `std::async`), dzięki czemu sprawdzanie rozmiaru zapisu nie blokuje pętli gry.
