# vendor/CMakeLists.txt
file(GLOB IMGUI_SOURCES
        "imgui/imgui.cpp"
        "imgui/imgui_draw.cpp"
        "imgui/imgui_tables.cpp"
        "imgui/imgui_widgets.cpp"
        "imgui/backends/imgui_impl_glfw.cpp"
        "imgui/backends/imgui_impl_vulkan.cpp"
)

add_library(ImGui STATIC ${IMGUI_SOURCES})

# Disable clang-tidy on ImGui sources
set_target_properties(ImGui PROPERTIES CXX_CLANG_TIDY "")

target_include_directories(ImGui SYSTEM PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
)

target_include_directories(ImGui SYSTEM PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        ${GLFW_DIR}/include
)
