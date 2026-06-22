# Kompresja RLE (Run-Length Encoding)

W celu zminimalizowania rozmiaru zajmowanego przez zapisane światy na dysku oraz przyspieszenia operacji wejścia-wyjścia (I/O), silnik [[api/VoxelEngine|VoxelEngine]] stosuje prosty i wydajny algorytm kompresji [[algorithms/Kompresja_RLE|RLE (Run-Length Encoding)]] do plików zapisów [[api/Chunk|chunków]] (`saves/world1/chunk_x_y_z.bin`).

Implementacja tego algorytmu znajduje się w pliku [Chunk.cpp](file:///c:/dev/repos/VoxelEngine/src/world/Chunk.cpp) w metodach [[api/Chunk|Chunk]]::save() oraz [[api/Chunk|Chunk]]::load().

---

## 💡 Zasada działania i Format Pliku

W świecie voxelowym bloki o tym samym typie często występują w dużych, zwartych grupach. Na przykład:
* Nad powierzchnią ziemi znajduje się ogromna, nieprzerwana przestrzeń powietrza (`BlockType::Air`).
* Głęboko pod ziemią znajdują się duże złoża litego kamienia (`BlockType::Stone`).

Algorytm **RLE** polega na zastąpieniu sekwencji powtarzających się takich samych danych jedną parą: **[Wartość][LiczbaWystąpień]**.

### Format binarny pojedynczej serii:
1. **Typ bloku (`BlockType`)** — 1 bajt (enum klasy `uint8_t`).
2. **Licznik serii (`count`)** — 4 bajty (`uint32_t`).

Każda zapisana seria danych zajmuje zatem dokładnie **5 bajtów** w pliku binarnym.

---

## 💾 1. Zapis [[api/Chunk|Chunku]] ([[api/Chunk|Chunk]]::save)

Podczas zapisu do pliku, pętla przemieszcza się trójwymiarowo po całej tablicy `m_blocks`:

```cpp
BlockType currentType = m_blocks[0][0][0];
uint32_t count        = 0;

for (const auto& slice: m_blocks) {
    for (const auto& row: slice) {
        for (const auto& block: row) {
            if (block == currentType) {
                count++;
            } else {
                out.write(reinterpret_cast<const char*>(&currentType), sizeof(BlockType));
                out.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));
                currentType = block;
                count       = 1;
            }
        }
    }
}
out.write(reinterpret_cast<const char*>(&currentType), sizeof(BlockType));
out.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));
```

* Jeśli blok jest taki sam jak poprzedni, zwiększany jest licznik `count`.
* Gdy typ bloku się zmienia, aktualna para `(currentType, count)` zostaje natychmiast zapisana do strumienia `std::ofstream`, a licznik zostaje zresetowany do $1$ dla nowego typu.

---

## 📂 2. Odczyt [[api/Chunk|Chunku]] ([[api/Chunk|Chunk]]::load)

Deserializacja (odczyt) polega na rekonstrukcji oryginalnej tablicy voxelowej z zakodowanych serii:

```cpp
int x = 0, y = 0, z = 0;
while (in && x < CHUNK_SIZE)
{
    BlockType type;
    uint32_t count;
    in.read(reinterpret_cast<char*>(&type), sizeof(BlockType));
    if (in.eof()) break;
    in.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));

    for (uint32_t i = 0; i < count; ++i)
    {
        m_blocks[x][y][z] = type;
        z++;
        if (z >= CHUNK_SIZE)
        {
            z = 0;
            y++;
            if (y >= CHUNK_SIZE)
            {
                y = 0;
                x++;
                if (x >= CHUNK_SIZE) break;
            }
        }
    }
}
```

Dla każdej wczytanej pary z pliku, pętla wewnętrzna przypisuje wczytany typ bloku do kolejnych komórek trójwymiarowej tablicy `m_blocks`, odpowiednio inkrementując indeksy `x`, `y`, `z`.

---

## 📈 Analiza Zysku Pamięciowego

Pojedynczy [[api/Chunk|chunk]] w pamięci RAM przechowuje:
$$64 \times 64 \times 64 = 262\ 144 \text{ voxelów}$$

Bez kompresji (zapisując surowy typ bloku jako 1 bajt):
$$\text{Rozmiar surowy} = 262\ 144 \text{ B} = 256 \text{ KB}$$

Dzięki kompresji RLE:
* **[[api/Chunk|Chunk]] w powietrzu** (same bloki `Air`):
  * Zapisywany jako jedna para: `[Air][262144]`.
  * Rozmiar pliku: **5 bajtów** (kompresja o **99.998%**).
* **[[api/Chunk|Chunk]] w głębi ziemi** (same bloki `Stone`):
  * Zapisywany jako jedna para: `[Stone][262144]`.
  * Rozmiar pliku: **5 bajtów** (kompresja o **99.998%**).
* **[[api/Chunk|Chunk]] na powierzchni** (mieszany: powietrze, trawa, ziemia, kamień):
  * W zależności od ukształtowania terenu generuje od kilkunastu do kilkuset serii.
  * Rozmiar pliku: zazwyczaj **1 KB - 5 KB** (kompresja o **98% - 99%**).

Mniejszy rozmiar zapisu bezpośrednio odciąża dysk twardy podczas dynamicznego ładowania terenu w locie, eliminując mikroprzycięcia (stuttering) gry.
