#pragma once

namespace voxl {

    class Log {
    public:
        // Na razie tylko przygotowuje konsolę
        static void init();

        // Tutaj w przyszłości dodasz np:
        // static void info(const std::string& message);
        // static void error(const std::string& message);
    };

} // namespace voxl