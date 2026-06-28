# Klasa Frustum

Klasa **`Frustum`** reprezentuje bryłę ostrosłupa ściętego widoku kamery. Służy do przeprowadzania testów widoczności [[api/Chunk|chunków]] w przestrzeni trójwymiarowej świata gry, co zapobiega wysyłaniu niewidocznych geometrii do GPU.

Definicja klasy znajduje się w pliku [Frustum.h](../../src/core/Frustum.h), a jej implementacja w [Frustum.cpp](../../src/core/Frustum.cpp).

---

## 🏗️ Definicja Klasy (`Frustum.h`)

```cpp
struct Plane
{
    glm::vec3 normal = {0.0f, 1.0f, 0.0f};
    float distance   = 0.0f;

    Plane() = default;

    Plane(const glm::vec4& p)
    {
        normal        = glm::vec3(p);
        const float l = glm::length(normal);
        normal        /= l;
        distance      = p.w / l;
    }
};

class Frustum
{
public:
    Frustum() = default;
    explicit Frustum(const glm::mat4& vpMatrix);

    void extractPlanes(const glm::mat4& vpMatrix);
    [[nodiscard]] bool intersectsAABB(const glm::vec3& minAABB, const glm::vec3& maxAABB) const;

private:
    std::array<Plane, 6> m_planes;
};
```

---

## 🔑 Kluczowe Metody i Ich Rola

### 1. `Plane(const glm::vec4& p)`
Konstruktor struktury płaszczyzny. Przyjmuje współczynniki płaszczyzny w postaci wektora 4D $(A, B, C, D)$, wylicza długość wektora normalnego ($l = \sqrt{A^2+B^2+C^2}$), a następnie normalizuje wektor normalny $\vec{n} = (A/l, B/l, C/l)$ i dzieli odległość $distance = D/l$. Znormalizowanie ułatwia obliczanie odległości w teście kolizji.

### 2. `void extractPlanes(const glm::mat4& vpMatrix)`
Wyodrębnia 6 płaszczyzn ograniczających bryłę widoku na podstawie macierzy widoku-projekcji ($VP$). Wykorzystuje do tego matematyczną metodę **Gribba-Hartmanna**, sumując lub odejmując wiersze transponowanej macierzy $VP$. Szczegółowy opis tego procesu i wzory matematyczne znajdziesz w artykule **[[algorithms/Frustum_Culling|Frustum Culling]]**.

### 3. `bool intersectsAABB(const glm::vec3& minAABB, const glm::vec3& maxAABB) const`
Sprawdza, czy prostopadłościan AABB otaczający dany [[api/Chunk|chunk]] leży choć częściowo wewnątrz bryły widoku.
Testuje każdy z 6 płaszczyzn z osobna, wyliczając wierzchołek pozytywny (p-vertex) AABB dla wektora normalnego danej płaszczyzny.
* Jeśli wierzchołek pozytywny znajduje się po ujemnej stronie choć jednej z płaszczyzn, metoda natychmiast zwraca `false` (chunk jest niewidoczny).
* Jeśli obiekt przejdzie pozytywnie test dla wszystkich płaszczyzn, zwraca `true`.

---

## 🔗 Relacje z innymi klasami skarbca

* **[[api/Camera|Camera]]** — płaszczyzny bryły widoku są wyliczane na podstawie macierzy widoku generowanej przez [[api/Camera|kamerę]].
* **[[api/VulkanContext|VulkanContext]]** — w metodzie `recordCommandBuffer` renderera, instancja `Frustum` jest tworzona w każdej klatce przed generowaniem komend rysowania w celu odrzucenia niewidocznych [[api/Chunk|chunków]].
* **[[algorithms/Frustum_Culling|Frustum Culling]]** — notatka teoretyczno-matematyczna szczegółowo opisująca algorytm działania klasy.
