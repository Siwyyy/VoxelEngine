# Frustum Culling (Odrzucanie bryły widoku)

**Frustum Culling** jest kluczową optymalizacją w potoku renderowania 3D silnika [[api/VoxelEngine|VoxelEngine]]. Zapobiega wysyłaniu [[api/Chunk|chunków]], które znajdują się poza polem widzenia kamery, do przetwarzania przez układ graficzny. 

Implementacja znajduje się w pliku [Frustum.cpp](../../src/core/Frustum.cpp) (klasy [[api/Frustum|Frustum]]).

---

## 📐 Matematyka Ekstrakcji Płaszczyzn (Metoda Gribba-Hartmanna)

Bryła widoku kamery ([[api/Frustum|Frustum]]) definiowana jest przez 6 płaszczyzn: lewą, prawą, dolną, górną, bliską (Near) oraz daleką (Far). Każda płaszczyzna jest opisana ogólnym równaniem matematycznym:
$$A \cdot x + B \cdot y + C \cdot z + D = 0$$

Wektor normalny płaszczyzny to $\vec{n} = (A, B, C)$, a odległość od początku układu współrzędnych to $D$.

W metodzie [[api/Frustum|Frustum]]::extractPlanes(), płaszczyzny są wyliczane bezpośrednio z macierzy widoku-projekcji ($VP$). Transponując macierz $VP$ do macierzy pomocniczej $M$, wyodrębniamy poszczególne wiersze (indeksowane od 0 do 3):

```cpp
glm::mat4 M = glm::transpose(vpMatrix);
m_planes[0] = Plane(M[3] + M[0]); // Lewa płaszczyzna
m_planes[1] = Plane(M[3] - M[0]); // Prawa płaszczyzna
m_planes[2] = Plane(M[3] + M[1]); // Dolna płaszczyzna
m_planes[3] = Plane(M[3] - M[1]); // Górna płaszczyzna
m_planes[4] = Plane(M[3] + M[2]); // Bliska płaszczyzna (Near)
m_planes[5] = Plane(M[3] - M[2]); // Daleka płaszczyzna (Far)
```

Konstruktor klasy `Plane` automatycznie normalizuje wektor $\vec{n}$, aby jego długość wynosiła $1$. Dzięki temu $A^2 + B^2 + C^2 = 1$, co ułatwia późniejsze obliczanie odległości punktów od płaszczyzny.

---

## 📦 Test Przecięcia bryły z AABB (intersectsAABB)

Aby sprawdzić, czy [[api/Chunk|chunk]] leży wewnątrz pola widzenia, silnik wykonuje test kolizji prostopadłościanu wyrównanego do osi (AABB – Axis-Aligned Bounding Box) z płaszczyznami frustum.

AABB chunku definiują dwa wektory trójwymiarowe:
* `minAABB` — dolny lewy tylny róg chunku.
* `maxAABB` — górny prawy przedni róg chunku.

Algorytm sprawdza każdy z 6 płaszczyzn z osobna. Zamiast testować wszystkie 8 wierzchołków sześcianu, wyznacza tzw. **wierzchołek pozytywny** (p-vertex) na podstawie znaku składowych wektora normalnego płaszczyzny:

```cpp
glm::vec3 pVertex = minAABB;
if (plane.normal.x >= 0) pVertex.x = maxAABB.x;
if (plane.normal.y >= 0) pVertex.y = maxAABB.y;
if (plane.normal.z >= 0) pVertex.z = maxAABB.z;
```

* Jeśli ten skrajny wierzchołek leży po "ujemnej" stronie płaszczyzny (czyli za płaszczyzną), cały sześcian znajduje się całkowicie poza polem widzenia, a test natychmiast zwraca fałsz (`false`).
* Odległość wierzchołka od płaszczyzny znormalizowanej oblicza się jako:
  $$d = \vec{n} \cdot \vec{p} + D$$
  gdzie $\vec{p}$ to współrzędne wierzchołka `pVertex`.

```cpp
if (glm::dot(plane.normal, pVertex) + plane.distance < 0.0f) { 
    return false; // Obiekt jest całkowicie niewidoczny
}
```

Jeśli obiekt przejdzie pozytywnie test dla wszystkich 6 płaszczyzn, jest uznawany za widoczny i trafia do [[vulkan/Krok_3_Potok_Graficzny|potoku renderowania]].

---

## 🚀 Zastosowanie w Renderowaniu

Test Frustum Culling jest wywoływany dynamicznie podczas nagrywania bufora poleceń Vulkan ([[api/VulkanContext|VulkanContext]]::recordCommandBuffer()). 

Dla każdego aktywnego [[api/Chunk|chunku]] wyliczane są współrzędne otaczające AABB w przestrzeni świata gry, a następnie wywoływany jest test:

```cpp
const glm::vec3 minAABB = chunk->getPosition(); // Klasa [[api/Chunk|Chunk]]
const glm::vec3 maxAABB = minAABB + glm::vec3(Chunk::CHUNK_SIZE);

if (!frustum.intersectsAABB(minAABB, maxAABB)) { // Klasa [[api/Frustum|Frustum]]
    continue; // Odrzuć chunk - nie jest wysyłany do GPU
}
```
Metoda ta pozwala zaoszczędzić setki tysięcy operacji renderowania wierzchołków na GPU w każdej klatce.
