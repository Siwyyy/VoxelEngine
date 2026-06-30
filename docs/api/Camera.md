# Klasa Camera

Klasa **`voxl::Camera`** reprezentuje wirtualnego obserwatora w trójwymiarowej przestrzeni gry, pełniąc jednocześnie rolę gracza. Wylicza macierz widoku (View Matrix) dla renderera oraz zarządza ruchem na podstawie akcji wejściowych.

Definicja klasy znajduje się w pliku [Camera.h](../../src/core/Camera.h), a jej implementacja w [Camera.cpp](../../src/core/Camera.cpp).

---

## 🏗️ Definicja Klasy (`Camera.h`)

```cpp
namespace voxl
{
    class Camera
    {
    public:
        Camera(glm::vec3 startPosition, glm::vec3 startUp, float startYaw, float startPitch, const ActionManager& actionManager);

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

        const ActionManager& m_actionManager;

        glm::vec3 m_position;
        glm::vec3 m_front{};
        glm::vec3 m_up{};
        glm::vec3 m_right{};
        glm::vec3 m_worldUp;

        float m_yaw;
        float m_pitch;

        float m_movementSpeed    = 1.4f; // 1.4 m/s (szybkość chodu)
        float m_mouseSensitivity = 0.1f;

        bool m_firstMouse = true;
        float m_lastX     = 0.0f;
        float m_lastY     = 0.0f;
    };
}
```

---

## 🔑 Kluczowe Metody i Ich Rola

### 1. `void update(float deltaTime)`
Aktualizuje pozycję kamery oraz jej obrót w każdej klatce:
* **Ruch**: Odpytuje referencję do `ActionManager` o stany powiązanych akcji ruchowych (`MoveForward`, `MoveBackward`, `MoveLeft`, `MoveRight`, `MoveUp`, `MoveDown`):
  * Akcja `MoveForward` / `MoveBackward` przesuwa kamerę do przodu/tyłu wzdłuż wektora patrzenia.
  * Akcja `MoveLeft` / `MoveRight` przesuwa kamerę na boki.
  * Akcja `MoveUp` / `MoveDown` przesuwa kamerę pionowo w osi świata.
* **Obroty**: Odpytuje bezpośrednio `Input::getMousePosition()`, wylicza przesunięcie kursora myszy od poprzedniej klatki i modyfikuje kąty Yaw oraz Pitch (ograniczony do zakresu $[-89^\circ, 89^\circ]$). Po aktualizacji kątów wywoływana jest metoda `updateCameraVectors()`.

### 2. `void updateCameraVectors()`
Wylicza wektory kierunków lokalnych kamery na podstawie aktualnych kątów Yaw i Pitch:
* Wektor patrzenia (`m_front`):
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

* **[[api/Input|Input]] / [[api/Input|ActionManager]]** — Klasa pobiera pozycję myszy z singletonu `Input`, a ruchy postaci weryfikuje poprzez powiązania w przekazanym obiekcie `ActionManager`.
* **[[api/VoxelEngine|VoxelEngine]]** — Pętla główna wywołuje `Camera::update` w każdej klatce i przekazuje macierz widoku do renderera.
* **[[api/Frustum|Frustum]]** — Płaszczyzny bryły widoku są generowane na podstawie macierzy widoku-projekcji, która korzysta z macierzy widoku kamery.
* **[[algorithms/Raycasting_DDA|Raycasting DDA]]** — Pozycja kamery i wektor patrzenia (`m_front`) definiują początek i kierunek promienia interakcji gracza z wokselami w świecie.
