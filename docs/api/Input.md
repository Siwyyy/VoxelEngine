# System Obsługi Wejścia (`voxl::Input`, `voxl::ActionManager`)

System obsługi wejścia w silniku odpowiada za przechwytywanie zdarzeń z klawiatury i myszy oraz ich mapowanie na logiczne akcje gry. Składa się z niskopoziomowego odpytywania stanu urządzeń (Singleton `Input`) oraz wysokopoziomowego menedżera akcji (`ActionManager`).

Definicje systemu znajdują się w katalogu [src/input/](../../src/input/):
* [Input.h](../../src/input/Input.h) oraz [GLFWInput.h](../../src/input/GLFWInput.h) / [GLFWInput.cpp](../../src/input/GLFWInput.cpp) – warstwa sprzętowa.
* [InputCodes.h](../../src/input/InputCodes.h) – kody klawiszy i przycisków myszy.
* [InputAction.h](../../src/input/InputAction.h) – definicja logicznych akcji.
* [ActionManager.h](../../src/input/ActionManager.h) / [ActionManager.cpp](../../src/input/ActionManager.cpp) – mapowanie i weryfikacja akcji.

---

## ⌨️ Kody Wejścia (`KeyCode`, `MouseCode`)

Silnik definiuje silnie typowane typy wyliczeniowe (enum class) dla klawiatury oraz myszy:

```cpp
namespace voxl
{
    enum class KeyCode : uint32_t
    {
        Space        = 32,
        A            = 65,
        // ... (kody klawiszy klawiatury zgodne z GLFW)
        LeftShift    = 340,
        LeftControl  = 341,
        Escape       = 256,
        Tab          = 258
    };

    enum class MouseCode : uint32_t
    {
        Left   = 1,
        Right  = 2,
        Middle = 3
        // ...
    };
}
```

---

## 🏗️ Odpytywanie Stanu Sprzętowego (`Input`, `GLFWInput`)

Klasa bazowa **`Input`** definiuje interfejs do sprawdzania surowego stanu klawiszy i myszy. Działa jako statyczny singleton (`s_instance`).

### Definicja interfejsu (`Input.h`)
```cpp
namespace voxl
{
    class Input
    {
    public:
        Input()                        = default;
        virtual ~Input()               = default;

        inline static bool isKeyPressed(KeyCode keyCode) { return s_instance->isKeyPressedImpl(keyCode); }
        inline static bool isMouseButtonPressed(MouseCode mouseCode) { return s_instance->isMouseButtonPressedImpl(mouseCode); }
        inline static glm::vec2 getMousePosition() { return s_instance->getMousePositionImpl(); }

    protected:
        virtual bool isKeyPressedImpl(KeyCode keycode) const = 0;
        virtual bool isMouseButtonPressedImpl(MouseCode button) const = 0;
        virtual glm::vec2 getMousePositionImpl() const = 0;

    public:
        static Input* s_instance;
    };
}
```

### Implementacja GLFW (`GLFWInput.cpp`)
Platformowa implementacja `GLFWInput` implementuje metody wirtualne, odpytując bezpośrednio bibliotekę GLFW:
```cpp
namespace voxl
{
    bool GLFWInput::isKeyPressedImpl(KeyCode keycode) const
    {
        auto state = glfwGetKey(m_window, static_cast<int32_t>(keycode));
        return state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    bool GLFWInput::isMouseButtonPressedImpl(MouseCode button) const
    {
        auto state = glfwGetMouseButton(m_window, static_cast<int32_t>(button));
        return state == GLFW_PRESS;
    }
}
```

---

## 🎯 System Akcji (`InputAction`, `ActionManager`)

Aby uniezależnić logikę gry (np. ruch kamery) od konkretnych fizycznych klawiszy, silnik definiuje pojęcie **akcji wejściowych**.

### Definicja Akcji (`InputAction.h`)
```cpp
namespace voxl
{
    enum class InputAction : uint32_t
    {
        MoveForward,
        MoveBackward,
        MoveLeft,
        MoveRight,
        MoveUp,
        MoveDown,
        Interact,
        SecondaryInteract,
        ToggleCursor,
        Exit
    };
}
```

### Menedżer Akcji (`ActionManager`)
Klasa `ActionManager` odpowiada za przypisywanie fizycznych kodów klawiszy do akcji oraz weryfikację, czy dana akcja jest wciśnięta. Akcja może być przypisana do typu `KeyCode` lub `MouseCode` (za pomocą typu `std::variant`).

```cpp
namespace voxl
{
    using InputBinding = std::variant<KeyCode, MouseCode>;

    class ActionManager
    {
    public:
        void bindAction(InputAction action, InputBinding binding);
        void clearAction(InputAction action);
        
        bool isActionPressed(InputAction action) const;

    private:
        std::unordered_map<InputAction, std::vector<InputBinding>> m_actionMap;
    };
}
```

Implementacja `isActionPressed` sprawdza wszystkie powiązane klawisze/przyciski dla danej akcji:
```cpp
bool ActionManager::isActionPressed(InputAction action) const
{
    auto it = m_actionMap.find(action);
    if (it == m_actionMap.end()) return false;

    return std::ranges::any_of(it->second, [](const InputBinding& binding)
    {
        if (std::holds_alternative<KeyCode>(binding))
        {
            return Input::isKeyPressed(std::get<KeyCode>(binding));
        }
        if (std::holds_alternative<MouseCode>(binding))
        {
            return Input::isMouseButtonPressed(std::get<MouseCode>(binding));
        }
        return false;
    });
}
```

---

## 🔗 Relacje z innymi klasami skarbca

* **[[api/VoxelEngine|VoxelEngine]]** — inicjalizuje instancję `GLFWInput` i rejestruje domyślne powiązania akcji (np. `bindAction(InputAction::MoveForward, KeyCode::W)`).
* **[[api/Camera|Camera]]** — pobiera referencję do `ActionManager` i sprawdza stany akcji ruchowych (`MoveForward`, `MoveLeft` itd.) zamiast bezpośrednio odpytywać stan klawiatury.
