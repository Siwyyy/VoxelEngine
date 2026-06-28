#include "Log.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace voxl
{
    void Log::init()
    {
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
#endif
    }
} // namespace voxl
