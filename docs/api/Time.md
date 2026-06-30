# Klasa Time

Klasa **`Time`** to pomocnicza, w pełni statyczna klasa użytkowa (utility class) odpowiedzialna za precyzyjne śledzenie czasu rzeczywistego w silniku. Oblicza czas trwania pojedynczej klatki (Delta Time) oraz całkowity czas działania aplikacji od momentu jej uruchomienia, bazując na standardowej bibliotece `<chrono>`.

Definicja klasy znajduje się w nagłówku [Time.h](../../src/core/Time.h).

---

## 🏗️ Definicja Klasy (`Time.h`)

```cpp
namespace voxl
{
    class Time
    {
    public:
        static void init();
        static void tick();

        [[nodiscard]] static float getDeltaTime();
        [[nodiscard]] static float getTime();

    private:
        inline static std::chrono::time_point<std::chrono::high_resolution_clock> s_startTime;
        inline static std::chrono::time_point<std::chrono::high_resolution_clock> s_lastTime;
        inline static float s_deltaTime = 0.0f;
        inline static float s_time      = 0.0f;
    };
}
```

---

## 🔑 Kluczowe Metody i Ich Rola

### 1. `static void init()`
Metoda wywoływana jednorazowo podczas uruchamiania pętli głównej silnika w [[api/VoxelEngine|VoxelEngine]]. Ustala punkt początkowy czasu gry (`s_startTime`) oraz czas ostatniej klatki (`s_lastTime`) na bieżący moment pobrany z zegara wysokiej rozdzielczości `std::chrono::high_resolution_clock`.

### 2. `static void tick()`
Metoda wywoływana na początku każdej klatki w głównej pętli renderowania. Pobiera aktualny punkt czasowy, oblicza czas, jaki upłynął od poprzedniego wywołania (`s_deltaTime`), i przypisuje go jako zmiennoprzecinkową wartość w sekundach. Aktualizuje również całkowity czas trwania silnika (`s_time`).

### 3. `static float getDeltaTime()`
Zwraca czas (w sekundach) trwania ostatniej klatki. Służy do uniezależnienia prędkości wykonywania fizyki, ruchów kamery (np. w [[api/Camera|Camera]]) oraz działania liczników (np. opóźnienia niszczenia/budowania bloków) od liczby klatek na sekundę (FPS).

### 4. `static float getTime()`
Zwraca czas (w sekundach) działania silnika od momentu wywołania metody `init()`. Zapewnia monotoniczny zegar wykorzystywany w logice gry i profilowaniu.

---

## 🔗 Relacje z innymi klasami skarbca

* **[[api/VoxelEngine|VoxelEngine]]** — główna klasa inicjalizuje obiekt `Time` na starcie pętli i wywołuje `Time::tick()` na początku każdej klatki, synchronizując aktualizację stanów silnika.
* **[[api/Camera|Camera]]** — wykorzystuje `Time::getDeltaTime()` do skalowania prędkości poruszania się kamery, zapobiegając przyspieszeniu/spowolnieniu ruchu przy zmiennym FPS.
