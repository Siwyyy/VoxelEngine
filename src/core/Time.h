#pragma once
#include <chrono>

class Time
{
public:
    /**
     * Inicjalizuje punkty czasowe startu oraz ostatniej klatki
     */
    static void init()
    {
        s_startTime = std::chrono::high_resolution_clock::now();
        s_lastTime  = s_startTime;
    }


    /**
     * Aktualizuje deltaTime oraz całkowity czas działania silnika
     */
    static void tick()
    {
        const auto currentTime = std::chrono::high_resolution_clock::now();

        s_deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - s_lastTime).count();
        s_lastTime  = currentTime;

        s_time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - s_startTime).count();
    }

    [[nodiscard]] static float getDeltaTime() { return s_deltaTime; }
    [[nodiscard]] static float getTime() { return s_time; }

private:
    inline static std::chrono::time_point<std::chrono::high_resolution_clock> s_startTime;
    inline static std::chrono::time_point<std::chrono::high_resolution_clock> s_lastTime;
    inline static float s_deltaTime = 0.0f;
    inline static float s_time      = 0.0f;
};
