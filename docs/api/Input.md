# Klasa Input i GLFWInput

Klasa **`Input`** oraz jej platformowa implementacja **`GLFWInput`** zarządzają wejściem od użytkownika (klawiatura, mysz). System ten opiera się na **wzorcu Singleton**, dzięki czemu stan klawiszy lub pozycję myszy można sprawdzić w dowolnym miejscu w kodzie silnika za pomocą prostego API statycznego.

Definicja interfejsu znajduje się w pliku [Input.h](file:///c:/dev/repos/VoxelEngine/src/input/Input.h), klasa dziedzicząca w [GLFWInput.h](file:///c:/dev/repos/VoxelEngine/src/input/GLFWInput.h), a jej implementacja w [GLFWInput.cpp](file:///c:/dev/repos/VoxelEngine/src/input/GLFWInput.cpp).

---

## 🏗️ Definicja Interfejsu Bazowego (`Input.h`)

```cpp
class Input
{
public:
    Input()                        = default;
    Input(const Input&)            = delete;
    Input& operator=(const Input&) = delete;
    Input(Input&&)                 = delete;
    Input& operator=(Input&&)      = delete;
    virtual ~Input()               = default;

    inline static bool isKeyPressed(const int32_t keycode) { return s_instance->isKeyPressedImpl(keycode); }
    inline static bool isMouseButtonPressed(const int32_t button) { return s_instance->isMouseButtonPressedImpl(button); }
    inline static glm::vec2 getMousePosition() { return s_instance->getMousePositionImpl(); }

protected:
    [[nodiscard]] virtual bool isKeyPressedImpl(int32_t keycode) const = 0;
    [[nodiscard]] virtual bool isMouseButtonPressedImpl(int32_t button) const = 0;
    [[nodiscard]] virtual glm::vec2 getMousePositionImpl() const = 0;

public:
    static Input* s_instance;
};
```

---

## ⚙️ Definicja Klasy GLFWInput (`GLFWInput.h`)

```cpp
class GLFWInput final : public Input
{
public:
    explicit GLFWInput(GLFWwindow* window);

private:
    [[nodiscard]] bool isKeyPressedImpl(int32_t keycode) const override;
    [[nodiscard]] bool isMouseButtonPressedImpl(int32_t button) const override;
    [[nodiscard]] glm::vec2 getMousePositionImpl() const override;

    GLFWwindow* m_window;
};
```

---

## 🔑 Implementacja Metod (`GLFWInput.cpp`)

### 1. Inicjalizacja Singletonu
Podczas tworzenia instancji `GLFWInput` w konstruktorze przypisywana jest statyczna instancja `s_instance`:
```cpp
GLFWInput::GLFWInput(GLFWwindow* window)
    : m_window(window)
{
    s_instance = this;
}
```

### 2. `bool isKeyPressedImpl(const int32_t keycode) const`
Odpytuje stan wybranego klawisza za pomocą funkcji GLFW `glfwGetKey`. Zwraca `true`, jeśli klawisz jest wciśnięty (`GLFW_PRESS`) lub przytrzymany (`GLFW_REPEAT`):
```cpp
bool GLFWInput::isKeyPressedImpl(const int32_t keycode) const
{
    const auto state = glfwGetKey(m_window, keycode);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}
```

### 3. `bool isMouseButtonPressedImpl(const int32_t button) const`
Sprawdza kliknięcie przycisków myszy przy użyciu `glfwGetMouseButton`. Zwraca `true` tylko w momencie wciśnięcia (`GLFW_PRESS`):
```cpp
bool GLFWInput::isMouseButtonPressedImpl(const int32_t button) const
{
    const auto state = glfwGetMouseButton(m_window, button);
    return state == GLFW_PRESS;
}
```

### 4. `glm::vec2 getMousePositionImpl() const`
Pobiera bezwzględne współrzędne kursora myszy na ekranie w pikselach:
```cpp
glm::vec2 GLFWInput::getMousePositionImpl() const
{
    double x_pos, y_pos;
    glfwGetCursorPos(m_window, &x_pos, &y_pos);
    return {static_cast<float>(x_pos), static_cast<float>(y_pos)};
}
```

---

## 🔗 Relacje z innymi klasami skarbca

* **[[api/Window|Window]]** — `GLFWInput` wymaga wskaźnika do okna GLFW w celu odpytywania stanów wejścia.
* **[[api/VoxelEngine|VoxelEngine]]** — silnik inicjalizuje obiekt `GLFWInput` na początku działania i odpytuje go o klawisz wyjścia (`ESC`) lub przełączenie trybu kursora (`TAB`).
* **[[api/Camera|Camera]]** — kamera odpytuje `Input::isKeyPressed` o ruchy postaci (`W, A, S, D, Space, Shift`) i pozycję myszy w każdej klatce.
