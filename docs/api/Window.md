# Klasa Window

Klasa **`Window`** to lekka klasa opakowująca bibliotekę GLFW. Odpowiada za utworzenie okna aplikacji, zarządzanie kontekstem systemu operacyjnego, przetwarzanie zdarzeń systemowych oraz integrację z API Vulkan.

Definicja klasy znajduje się w pliku [Window.h](../../src/core/Window.h), a jej implementacja w [Window.cpp](../../src/core/Window.cpp).

---

## 🏗️ Definicja Klasy (`Window.h`)

```cpp
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window
{
public:
    Window(uint32_t width, uint32_t height, const std::string& title);
    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    [[nodiscard]] bool shouldClose() const;
    void pollEvents();

    [[nodiscard]] GLFWwindow* getGLFWwindow() const { return m_window; }

private:
    void initWindow();

    uint32_t m_width;
    uint32_t m_height;
    std::string m_title;
    GLFWwindow* m_window;
};
```

---

## 🔑 Kluczowe Metody i Ich Rola

### 1. `Window(uint32_t width, uint32_t height, const std::string& title)`
Konstruktor klasy. Przypisuje wymiary, tytuł okna oraz wywołuje wewnętrzną metodę inicjalizującą GLFW: `initWindow()`.

### 2. `void initWindow()`
Inicjalizuje bibliotekę GLFW. Ponieważ aplikacja korzysta z API Vulkan (a nie z domyślnego OpenGL), przed utworzeniem okna wywoływane jest polecenie wyłączające kontekst OpenGL:
```cpp
glfwInit();
glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Informacja dla GLFW o braku kontekstu OpenGL
glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);  // Blokada zmiany rozmiaru okna
```
Następnie tworzy okno za pomocą `glfwCreateWindow` i zapisuje wskaźnik do niego w polu `m_window`.

### 3. `~Window()`
Destruktor klasy. Odpowiada za poprawne zamknięcie okna i zwolnienie zasobów biblioteki GLFW:
```cpp
glfwDestroyWindow(m_window);
glfwTerminate();
```

### 4. `bool shouldClose() const`
Zwraca informację, czy system operacyjny lub użytkownik zażądał zamknięcia okna (np. klikając przycisk X na pasku tytułowym). Opiera się na funkcji GLFW `glfwWindowShouldClose`.

### 5. `void pollEvents()`
Wywołuje przetwarzanie zdarzeń systemowych (takich jak ruchy myszką, kliknięcia, naciśnięcia klawiszy, zdarzenia systemowe okna) za pomocą:
```cpp
glfwPollEvents();
```

---

## 🔗 Relacje z innymi klasami skarbca

* **[[api/VoxelEngine|VoxelEngine]]** — klasa główna silnika tworzy instancję `Window` na początku swojego konstruktora i odpytuje metodę `pollEvents()` w każdej klatce pętli gry.
* **[[api/Input|Input]]** — implementacja klasy `GLFWInput` przyjmuje surowy wskaźnik `GLFWwindow*` w celu monitorowania stanów klawiatury i myszy.
* **[[api/VulkanContext|VulkanContext]]** — wskaźnik `GLFWwindow*` jest wymagany podczas tworzenia powierzchni renderowania `VkSurfaceKHR` oraz podczas inicjalizacji biblioteki ImGui.
