#include "core/VoxelEngine.h"

int main()
{
    try
    {
        VoxelEngine engine;
        engine.run();
    }
    catch (const std::exception& e)
    {
        std::println(stderr, "Fatal Error: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
