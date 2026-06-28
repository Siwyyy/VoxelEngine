# Algorytm Greedy Meshing

Algorytm **Greedy Meshing** jest jedną z najważniejszych optymalizacji w silniku [[api/VoxelEngine|VoxelEngine]]. Odpowiada za scalanie sąsiednich ścian voxelów tego samego typu w większe prostokąty (quady). Pozwala to na dramatyczne zmniejszenie liczby wierzchołków i indeksów przesyłanych do pamięci GPU.

Implementacja tego algorytmu znajduje się w pliku [Chunk.cpp](../../src/world/Chunk.cpp) w metodzie [[api/Chunk|Chunk]]::buildMesh().

---

## 💡 Koncepcja i Problematyka

W najprostszym (naiwnym) rendererze voxelowym każdy sześcian reprezentowany jest przez 6 ścian, a każda ściana to 2 trójkąty (4 wierzchołki i 6 indeksów). Przy rozmiarze [[api/Chunk|chunku]] $64 \times 64 \times 64 = 262\ 144$ bloków, naiwne renderowanie mogłoby wygenerować ponad 3 miliony trójkątów na jeden [[api/Chunk|chunk]].

**Greedy Meshing** rozwiązuje ten problem w dwóch krokach:
1. **Face Culling**: Ściany stykające się ze sobą bezpośrednio wewnątrz [[api/Chunk|chunku]] są niewidoczne, więc są pomijane.
2. **Merging (Scalanie)**: Widoczne ściany leżące w tej samej płaszczyźnie i mające ten sam typ bloku są łączone w jeden duży prostokąt.

---

## 🛠️ Przebieg Algorytmu Krok po Kroku

Algorytm przetwarza trójwymiarową przestrzeń wokseli w trzech głównych kierunkach (osiach $d \in \{0, 1, 2\}$, co odpowiada osiom $X, Y, Z$).

### Krok 1: Określenie osi pomocniczych
Dla wybranego głównego kierunku $d$ (np. oś $X$), wyznaczane są dwie osie prostopadłe $u$ oraz $v$ przy użyciu arytmetyki modularnej:
```cpp
int u = (d + 1) % 3;
int v = (d + 2) % 3;
```
Definiowany jest również wektor $q$ wskazujący sąsiedni blok w kierunku $d$ (np. $q = \{1, 0, 0\}$ dla $d=0$).

### Krok 2: Tworzenie Maski Plasterka (Slice Mask)
Algorytm przesuwa się plaster po plastrze wzdłuż osi $d$ (od $x[d] = -1$ do $dim\_d$). W każdym plasterku tworzy dwuwymiarową maskę `mask` o wymiarach $dim\_u \times dim\_v$.
Maska przechowuje informacje o tym, czy na granicy dwóch sąsiednich bloków istnieje widoczna ściana.

```cpp
Block blockCurrent = getBlock(x[0], x[1], x[2]);
Block blockCompare = getBlock(x[0] + q[0], x[1] + q[1], x[2] + q[2]);

bool currentSolid = blockCurrent != BlockType::Air;
bool compareSolid = blockCompare != BlockType::Air;

if (currentSolid != compareSolid)
{
    bool isBackFace  = !currentSolid;
    mask[x[v]][x[u]] = {
        .block = currentSolid ? blockCurrent : blockCompare, 
        .backFace = isBackFace
    };
}
else
{
    mask[x[v]][x[u]] = {.block = BlockType::Air, .backFace = false};
}
```

### Krok 3: Generowanie Prostokątów z Maski
Po obliczeniu maski dla danego plasterka, algorytm skanuje maskę wiersz po wierszu:
1. Znajduje niepusty element maski `c` na pozycji `[j][i]`.
2. **Skanowanie szerokości (`w`)**: Sprawdza, jak daleko w prawo (wzdłuż osi $u$) rozciągają się identyczne elementy maski:
   ```cpp
   for (w = 1; i + w < dim_u && mask[j][i + w] == c; ++w) {}
   ```
3. **Skanowanie wysokości (`h`)**: Sprawdza, jak głęboko w dół (wzdłuż osi $v$) można rozciągnąć ten pasek o szerokości `w`, tak aby wszystkie komórki w nowo powstałym prostokącie były identyczne z `c`:
   ```cpp
   bool done = false;
   for (h = 1; j + h < dim_v; ++h) {
       for (k = 0; k < w; ++k) {
           if (mask[j + h][i + k] != c) { done = true; break; }
       }
       if (done) break;
   }
   ```
4. **Tworzenie wierzchołków**: Gdy prostokąt o wymiarach `w` na `h` zostanie zdefiniowany, wyznaczane są współrzędne 4 wierzchołków (`v1`, `v2`, `v3`, `v4`) w przestrzeni 3D.
5. **Czyszczenie maski**: Wyrenderowany obszar maski jest czyszczony (ustawiany na `BlockType::Air`), aby nie przetwarzać go ponownie:
   ```cpp
   for (l = 0; l < h; ++l) {
       for (k = 0; k < w; ++k) {
           mask[j + l][i + k] = {.block = BlockType::Air, .backFace = false};
       }
   }
   ```
6. Przesuwa indeks kolumny `i` o szerokość `w` i kontynuuje skanowanie.

---

## 📈 Wydajność algorytmu w liczbach

Na podstawie pomiarów wydajności w [[api/VoxelEngine|VoxelEngine]], [[algorithms/Greedy_Meshing|Greedy Meshing]] redukuje dane geometryczne w następujący sposób:

* **Płaski teren wokselowy (64x64x64)**:
  * Naiwna siatka: ok. **150 000** trójkątów.
  * Zastosowanie [[algorithms/Greedy_Meshing|Greedy Meshing]]: ok. **12 000** trójkątów (redukcja o **92%**).
* **Zalety**:
  * Znacznie niższe zużycie pamięci VRAM GPU.
  * Mniej danych wierzchołków do przesłania przez magistralę PCIe.
  * Lepsza wydajność bufora głębokości (Early-Z) dzięki zredukowanej liczbie prymitywów.
