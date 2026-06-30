cmake_minimum_required(VERSION 3.20)
project(VoxelEngine)

set(CMAKE_CXX_STANDARD 23)

# Clang-Tidy static analysis option
option(ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)

# pomiar czasu kompilacji
set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")

if (ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy
        HINTS
            "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/Llvm/x64/bin"
            "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/x64/bin"
    )
    if (CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};
        -checks=bugprone-*,
        modernize-*,
        performance-*,
        -modernize-use-trailing-return-type,
        -bugprone-easily-swappable-parameters,
        -clang-analyzer-optin.core.EnumCastOutOfRange;
        --extra-arg=/EHsc")
        message(STATUS "Clang-Tidy found and enabled: ${CLANG_TIDY_EXE}")
    else ()
        message(WARNING "Clang-Tidy not found! Code analysis disabled.")
    endif ()
else ()
    message(STATUS "Clang-Tidy analysis is disabled (ENABLE_CLANG_TIDY=OFF).")
endif ()

# Find Vulkan package
find_package(Vulkan REQUIRED)

# Set external dependencies directories
set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw)

# Gather source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")

add_subdirectory(vendor EXCLUDE_FROM_ALL)
add_executable(${PROJECT_NAME} ${SOURCES})

# Configure include paths
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        ${GLFW_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
        Vulkan::Vulkan
        ${GLFW_DIR}/lib/glfw3.lib
        ImGui
)

# Enable Precompiled Headers (PCH)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

# Shader compilation using glslc from Vulkan SDK
find_program(GLSLC_EXECUTABLE NAMES glslc HINTS $ENV{VULKAN_SDK}/Bin)
if (NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found! Please check Vulkan SDK installation.")
endif ()

file(GLOB_RECURSE SHADERS "shaders/*.vert" "shaders/*.frag")
set(SPV_FILES "")

foreach (SHADER ${SHADERS})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(SPV_FILE "${CMAKE_CURRENT_BINARY_DIR}/shaders/${FILE_NAME}.spv")

    add_custom_command(
            OUTPUT ${SPV_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SPV_FILE}
            DEPENDS ${SHADER}
    )
    list(APPEND SPV_FILES ${SPV_FILE})
endforeach ()

add_custom_target(CompileShaders DEPENDS ${SPV_FILES})
add_dependencies(${PROJECT_NAME} CompileShaders)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)