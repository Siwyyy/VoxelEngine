# Klasa Camera

Klasa **`Camera`** reprezentuje wirtualnego obserwatora w trójwymiarowej przestrzeni gry, pełniąc jednocześnie rolę gracza. Implementuje system kamery typu **Euler Angles** (Pitch i Yaw) w przestrzeni o 6 stopniach swobody (6DoF) i wylicza macierz widoku (View Matrix) dla renderera.

Definicja klasy znajduje się w pliku [Camera.h](../../src/core/Camera.h), a jej implementacja w [Camera.cpp](../../src/core/Camera.cpp).

---

## 🏗️ Definicja Klasy (`Camera.h`)

```cpp
class Camera
{
public:
    Camera(glm::vec3 startPosition, glm::vec3 startUp, float startYaw, float startPitch);

    void update(float deltaTime);

    [[nodiscard]] glm::mat4 getViewMatrix() const;
    [[nodiscard]] glm::vec3 getPosition() const { return m_position; }
    [[nodiscard]] glm::vec3 getFront() const { return m_front; }
    [[nodiscard]] float getYaw() const { return m_yaw; }
    [[nodiscard]] float getPitch() const { return m_pitch; }
    void setPosition(const glm::vec3& pos) { m_position = pos; }

    void setRotation(const float yaw, const float pitch)
    {
        m_yaw   = yaw;
        m_pitch = pitch;
        updateCameraVectors();
    }

    void resetMouse() { m_firstMouse = true; }

private:
    void updateCameraVectors();

    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yaw;
    float m_pitch;

    float m_movementSpeed    = 1.4f; // 1.4 m/s (szybkość chodu)
    float m_mouseSensitivity = 0.1f;

    bool m_firstMouse = true;
    float m_lastX     = 0.0f;
    float m_lastY     = 0.0f;
};
```

---

## 🔑 Kluczowe Metody i Ich Rola

### 1. `void update(float deltaTime)`
Aktualizuje pozycję kamery oraz jej kąty obrotu w każdej klatce. Pobiera stan wejściowy ruchu (klawisze `W`, `A`, `S`, `D`, `Space` i `Left Shift`) oraz przesunięcie myszy za pomocą statycznych metod klasy [[api/Input|Input]]:

* **Ruch**:
  * Klawisze `W/S`: ruch do przodu/tyłu wzdłuż wektora patrzenia rzutowanego na płaszczyznę poziomą (dla zachowania stałej wysokości chodu).
  * Klawisze `A/D`: ruch na boki wzdłuż wektora `m_right`.
  * `Space / Left Shift`: ruch bezpośrednio w osi pionowej świata ($Y$).
* **Obroty myszą**:
  * Pobiera pozycję kursora myszy, wylicza offsety względem poprzedniej klatki, mnoży przez `m_mouseSensitivity` i aktualizuje kąty obrotu.
  * Ogranicza Pitch w zakresie $[-89^\circ, 89^\circ]$ i wywołuje `updateCameraVectors()`.

### 2. `void updateCameraVectors()`
Wylicza wektory kierunków lokalnych kamery na podstawie kątów Euler:

* Wejściowy wektor patrzenia (`m_front`):
  $$Front_x = \cos(\text{Yaw}) \cdot \cos(\text{Pitch})$$
  $$Front_y = \sin(\text{Pitch})$$
  $$Front_z = \sin(\text{Yaw}) \cdot \cos(\text{Pitch})$$
* Lokalny wektor w prawo (`m_right`):
  $$\vec{Right} = \text{normalize}(\vec{Front} \times \vec{WorldUp})$$
* Lokalny wektor w górę (`m_up`):
  $$\vec{Up} = \text{normalize}(\vec{Right} \times \vec{Front})$$

### 3. `glm::mat4 getViewMatrix() const`
Zwraca macierz widoku ($ViewMatrix$) wyliczoną na podstawie aktualnej pozycji i wektorów kierunku za pomocą funkcji `glm::lookAt`:
```cpp
glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(m_position, m_position + m_front, m_up);
}
```

---

## 🔗 Relacje z innymi klasami skarbca

* **[[api/Input|Input]]** — Klasa pobiera pozycje myszy i stan klawiszy za pomocą API klasy [[api/Input|Input]] do obsługi sterowania.
* **[[api/VoxelEngine|VoxelEngine]]** — Pętla główna wywołuje `Camera::update` w każdej klatce i przekazuje uzyskaną macierz widoku do renderera.
* **[[api/Frustum|Frustum]]** — Płaszczyzny bryły widoku są generowane na podstawie View-Projection Matrix, która częściowo składa się z macierzy widoku kamery.
* **[[algorithms/Raycasting_DDA|Raycasting DDA]]** — Pozycja kamery i wektor frontu są początkiem i kierunkiem promienia interakcji gracza z voxelami.
