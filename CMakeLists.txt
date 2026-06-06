cmake_minimum_required(VERSION 3.20)
project(VoxelEngine)

set(CMAKE_CXX_STANDARD 20)

option(ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=bugprone-*,modernize-*,performance-*,clang-diagnostic-*,-modernize-use-trailing-return-type")
        message(STATUS "Clang-Tidy found and enabled: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "Clang-Tidy not found! Code analysis disabled.")
    endif()
else()
    message(STATUS "Clang-Tidy analysis is disabled (ENABLE_CLANG_TIDY=OFF).")
endif()

find_package(Vulkan REQUIRED)

set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw)
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui)
set(VMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/vma)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h" "vendor/*.h")
file(GLOB IMGUI_SOURCES 
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES})

include(FetchContent)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        0.9.9.8
)
FetchContent_MakeAvailable(glm)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${Vulkan_INCLUDE_DIRS}
    ${GLFW_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${VMA_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Vulkan::Vulkan
    ${GLFW_DIR}/lib/glfw3.lib
    glm::glm
)

find_program(GLSLC_EXECUTABLE NAMES glslc HINTS $ENV{VULKAN_SDK}/Bin)
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found! Please check Vulkan SDK installation.")
endif()

file(GLOB_RECURSE SHADERS "shaders/*.vert" "shaders/*.frag")
set(SPV_FILES "")

foreach(SHADER ${SHADERS})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(SPV_FILE "${CMAKE_CURRENT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
    
    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SPV_FILE}
        DEPENDS ${SHADER}
    )
    list(APPEND SPV_FILES ${SPV_FILE})
endforeach()

add_custom_target(CompileShaders DEPENDS ${SPV_FILES})
add_dependencies(${PROJECT_NAME} CompileShaders)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)