# 🌍 System Świata i Zarządzanie Chunkami

W silniku voxelowym cały trójwymiarowy świat jest podzielony na mniejsze fragmenty zwane [[api/Chunk|chunkami]]. Pozwala to na wydajne renderowanie, asynchroniczne ładowanie danych w locie (streaming) i szybką modyfikację terenu przez gracza.

---

## 📐 1. Reprezentacja Przestrzenna i Koordynaty

Jednostką podstawową w silniku jest voxel (blok) reprezentowany przez strukturę `Block`. Składa się ona z typu bloku (`BlockType`) i 1 bajtu metadanych.
Wielkość fizyczna pojedynczego voxela wynosi **0.05 jednostki** świata gry.

### Rozmiar i Definicja Chunku ([[api/Chunk|Chunk]])
* **Rozmiar chunku**: `[[api/Chunk|Chunk]]::CHUNK_SIZE = 64`. Oznacza to, że pojedynczy [[api/Chunk|chunk]] zawiera $64 \times 64 \times 64 = 262\ 144$ voxeli.
* **Koordynaty chunku (ChunkCoord)**:
  Struktura opisująca pozycję [[api/Chunk|chunku]] w sieci trójwymiarowej za pomocą trzech liczb całkowitych `{x, y, z}`.
  Posiada własną strukturę haszującą `ChunkCoordHash` (zdefiniowaną w [[api/World|World.h]]), co pozwala na przechowywanie [[api/Chunk|chunków]] w tablicy mieszającej `std::unordered_map` wewnątrz klasy [[api/World|World]]:
  ```cpp
  std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> m_chunkMap;
  ```

---

## 🧵 2. Asynchroniczny Potok Streamingowy i Pula Wątków

Generowanie terenu i budowanie siatek wierzchołków (meshing) to operacje kosztowne obliczeniowo. Aby zapobiec zamrażaniu pętli głównej gry, silnik używa dedykowanej puli wątków **`ThreadPool`** (ograniczonej sprzętowo do `std::max(2, hardware_concurrency - 2)` wątków roboczych, co zapobiega przepełnieniu stosu przy wielu alokacjach).

### Przepływ Ładowania Chunku:
1. **Detekcja odległości**: W każdym kroku [[api/World|World]]::update obliczana jest odległość w [[api/Chunk|chunkach]] od gracza.
2. **Kolejkowanie w tle**: [[api/Chunk|Chunki]] w zasięgu renderowania (`m_renderDistance`), które nie istnieją w `m_chunkMap` (zarządzanej w [[api/World|World]]), są zlecane do załadowania/wygenerowania na puli wątków:
   ```cpp
   m_chunkFutures[coord] = m_threadPool->enqueue([pos, vb, ib, filepath]() {
       // Asynchroniczne ładowanie z dysku, generowanie szumu terenu i budowanie siatki
   });
   ```
3. **Synchronizacja**: W [[api/VoxelEngine|pętli głównej]] wyniki z wątków pobierane są asynchronicznie za pomocą sprawdzenia gotowości obiektów `std::future`.
4. **Czyszczenie i Garbage Collection**: [[api/Chunk|Chunki]], które znajdą się poza zasięgiem renderowania, są kolejkowane do zapisu na dysku. Aby zapobiec wyścigom CPU-GPU (gdy GPU wciąż odczytuje dane wierzchołków, które CPU próbuje usunąć), alokacja [[api/MegaBuffer|MegaBuffer]] jest uwalniana z opóźnieniem **3 klatek** za pomocą kolejki `m_chunksToDelete`.

---

## 🪚 3. Algorytm [[algorithms/Greedy_Meshing|Greedy Meshing]]

Tradycyjne renderowanie (gdzie każdy sześcian ma 6 osobnych ścian po 2 trójkąty każda) generowałoby miliony niepotrzebnych wierzchołków (np. wewnątrz terenu). Silnik implementuje dwie techniki optymalizacji geometrii:

### A. Face Culling (Culling Wewnętrzny)
Renderowane są tylko te ściany voxelów, które bezpośrednio stykają się z powietrzem (`BlockType::Air`). Ściany wewnętrzne, otoczone innymi stałymi blokami, są całkowicie pomijane.

### B. [[algorithms/Greedy_Meshing|Greedy Meshing (Scalanie Ścian)]]
Zaawansowany algorytm, który skanuje warstwy voxelów w trzech osiach i łączy sąsiednie, identyczne typy ścian w pojedyncze, większe prostokąty (quady).

#### Wizualizacja procesu scalania:
```
  [Naive Culling]              [Greedy Meshing]
  ┌───┬───┬───┬───┐            ┌───────────────┐
  │ G │ G │ G │ G │            │               │
  ├───┼───┼───┼───┤            │     Grass     │
  │ G │ G │ G │ G │    ───>    │    (1 Quad)   │
  ├───┼───┼───┼───┤            │               │
  │ G │ G │ G │ G │            │               │
  └───┴───┴───┴───┘            └───────────────┘
  12 wierzchołków, 4 quady     4 wierzchołki, 1 quad
```

Dzięki temu algorytmowi liczba generowanych wierzchołków i indeksów przesyłanych do [[api/MegaBuffer|MegaBuffer]] zmniejsza się o **ponad 90%**, co drastycznie odciąża procesor graficzny.

---

## 💾 4. Zapis Świata: Algorytm [[algorithms/Kompresja_RLE|RLE (Run-Length Encoding)]]

Pliki binarne chunków (`chunk_x_y_z.bin`) są zapisywane w formacie kompresji **RLE (Run-Length Encoding)**. Ponieważ większość objętości chunku pod ziemią to lita skała (`Stone`), a nad ziemią to powietrze (`Air`), RLE osiąga bardzo wysokie stopnie kompresji.

* **Zapis**: Algorytm liniowo odczytuje siatkę voxelów `m_blocks`. Zamiast pisać każdy voxel z osobna, grupuje identyczne typy bloków występujące po sobie i zapisuje parę: `[TypBloku (1B)][LiczbaWystąpień (4B)]`.
* **Odczyt**: Pętla odtwarzająca dekoduje pary, wypełniając trójwymiarową tablicę `m_blocks` odpowiednimi wartościami aż do całkowitego wypełnienia rozmiaru chunku.

---

## 🏹 5. Interakcja z voxelami ([[algorithms/Raycasting_DDA|Raycasting DDA]])

Kiedy gracz naciska przycisk myszy w celu usunięcia lub postawienia bloku, silnik rzuca promień (ray) z pozycji [[api/Camera|kamery]] w kierunku jej patrzenia. Do lokalizacji trafionego voxela używany jest szybki algorytm **[[algorithms/Raycasting_DDA|DDA (Digital Differential Analysis)]]**.

1. **Transformacja współrzędnych**: Pozycja kamery jest konwertowana na koordynaty voxelowe gry (skalowanie przez `1 / 0.05f`).
2. **Krok Raycastingu**: Algorytm iteracyjnie przechodzi po siatce bloków, sprawdzając kolizję z najbliższymi ścianami voxeli (krok po krokach `stepX`, `stepY`, `stepZ` na podstawie odległości promienia).
3. **Modyfikacja**:
   * **Lewy przycisk**: Ustawia trafiony blok na `BlockType::Air` (niszczenie).
   * **Prawy przycisk**: Ustawia blok przyległy do trafionej ściany na `BlockType::Stone` (budowanie).
4. **Oznaczenie jako Dirty**: Zmodyfikowany [[api/Chunk|chunk]] oraz [[api/Chunk|chunki]] sąsiednie (w przypadku modyfikacji na granicy) są oznaczane jako `m_isDirty = true` i `m_isSaveDirty = true`, co wymusza natychmiastową przebudowę ich siatki geometrycznej i przygotowanie do zapisu na dysku.
