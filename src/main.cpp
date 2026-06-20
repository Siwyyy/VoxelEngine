#include <iostream>

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
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
