# ⌨️ System Wejścia i Interakcja

System wejścia w [[api/VoxelEngine|VoxelEngine]] odpowiada za przechwytywanie zdarzeń od użytkownika (klawiatura, mysz) oraz przekładanie ich na ruch kamery w przestrzeni 3D oraz interakcję z voxelami w świecie gry.

---

## 🏛️ 1. Architektura Systemu Wejścia

System wejścia opiera się na **wzorcu Singleton** oraz polimorfizmie, oddzielając abstrakcję obsługi wejścia od konkretnej implementacji biblioteki okna (w tym przypadku GLFW).

* **Klasa bazowa [[api/Input|Input]]**:
  * Definiuje interfejs i przechowuje statyczny wskaźnik do instancji `s_instance`.
  * Udostępnia statyczne metody pomocnicze, które mogą być wywoływane w dowolnym miejscu w kodzie:
    * `Input::isKeyPressed(keycode)`
    * `Input::isMouseButtonPressed(button)`
    * `Input::getMousePosition()`
* **Klasa [[api/Input|GLFWInput]]**:
  * Konkretna implementacja bazująca na bibliotece GLFW.
  * Pobiera stan klawiszy bezpośrednio z okna GLFW za pomocą funkcji `glfwGetKey` i `glfwGetMouseButton`.
  * Śledzi pozycję kursora myszy za pomocą `glfwGetCursorPos`.

---

## 🎮 2. Klawiszologia i Przypisanie Klawiszy (Keybindings)

Kody klawiszy są zmapowane w plikach nagłówkowych `src/input/KeyCodes.h` oraz `MouseButtonCodes.h` pod wspólnym prefiksem `LAVA_KEY_*`. 

| Klawisz | Akcja w Grze | Opis |
| :--- | :--- | :--- |
| **W / S** | Ruch w przód / w tył | Przemieszcza kamerę wzdłuż jej wektora kierunku na płaszczyźnie poziomej. |
| **A / D** | Ruch w lewo / w prawo | Przemieszcza kamerę na boki (krok w bok - strafe). |
| **Space** | Ruch w górę | Podnosi pozycję kamery pionowo do góry. |
| **Left Shift** | Ruch w dół | Obniża pozycję kamery pionowo w dół. |
| **TAB** | Przełączanie kursora | Zwalnia kursor myszy z okna gry, umożliwiając interakcję z GUI ImGui. Ponowne naciśnięcie blokuje kursor w trybie gry. |
| **ESC** | Wyjście z gry | Inicjuje procedurę zamykania okna i bezpieczny zapis świata. |

---

## 👁️ 3. Obsługa Myszy i Ruch Kamery (Look-Around)

Kiedy kursor myszy jest przechwycony przez okno (`GLFW_CURSOR_DISABLED`), ruchy myszą sterują obrotem kamery.

1. **Pobieranie pozycji**: W klasie [[api/Camera|Camera]] pobierana jest aktualna pozycja kursora za pomocą `Input::getMousePosition()`.
2. **Obliczanie offsetu**: Różnica pozycji kursora między bieżącą a poprzednią klatką definiuje przesunięcie myszy (`xOffset`, `yOffset`).
3. **Modyfikacja obrotu**:
   * Przesunięcia są mnożone przez czułość myszy (`m_mouseSensitivity`).
   * Zmieniają one kąty **Yaw** (obrót w poziomie) oraz **Pitch** (obrót w pionie).
   * Kąt **Pitch** jest ograniczony (clamped) w zakresie od `-89.0f` do `89.0f` stopni, aby zapobiec zjawisku gimbal lock i "odwróceniu" kamery.
4. **Aktualizacja wektorów**: Na podstawie trygonometrii obliczany jest nowy wektor kierunku patrzenia kamery (`Front`):
   $$\text{Front.x} = \cos(\text{Yaw}) \cdot \cos(\text{Pitch})$$
   $$\text{Front.y} = \sin(\text{Pitch})$$
   $$\text{Front.z} = \sin(\text{Yaw}) \cdot \cos(\text{Pitch})$$

---

## ⛏️ 4. Interakcja z voxelami (Budowanie i Niszczenie)

Detekcja kliknięć myszy odbywa się w pętli głównej [[api/VoxelEngine|VoxelEngine]]::mainLoop() pod warunkiem, że kursor myszy jest zablokowany (gracz patrzy na świat):

```cpp
if (m_clickCooldown > 0.0f) {
    m_clickCooldown -= deltaTime;
} else {
    bool leftClick  = Input::isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
    bool rightClick = Input::isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);

    if (leftClick || rightClick) {
        m_world->processPlayerInteraction(m_camera->getPosition(), m_camera->getFront(), leftClick, rightClick);
        m_clickCooldown = 0.2f; // Cooldown 200 ms zapobiega wielokrotnemu kliknięciu w jednej klatce
    }
}
```

* **Cooldown (0.2s)**: Zapobiega błyskawicznemu niszczeniu lub stawianiu dziesiątek bloków naraz w wyniku jednego kliknięcia.
* **Kliknięcie Lewe (Niszczenie)**: Promień [[algorithms/Raycasting_DDA|DDA]] wyszukiwany w [[api/World|World]] oznacza trafiony voxel na `BlockType::Air` (zdefiniowany w [[api/Chunk|Chunk.h]]).
* **Kliknięcie Prawe (Stawianie)**: Promień [[algorithms/Raycasting_DDA|DDA]] wykrywa, z której strony uderzył w voxel (np. od góry, od dołu, z boku), i stawia nowy blok `BlockType::Stone` na sąsiedniej wolnej pozycji.

Więcej o algorytmie śledzenia promienia w siatce voxelowej ([[algorithms/Raycasting_DDA|DDA Raycasting]]) przeczytasz w rozdziale **[[03_System_Swiata_i_Chunki]]**.
