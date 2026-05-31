# VoxelEngine

A high-performance voxel graphics engine written in C++20 utilizing the modern Vulkan 1.3 API. The software architecture focuses on processing and visualizing volumetric data (voxels), minimizing driver overhead, and maximizing computational efficiency.

The engine serves as a technological foundation for native sandbox applications and specialized tools for visualizing 3D volumetric data.

## ✨ Key Features

- **Modern Graphics Pipeline**: Vulkan 1.3 pipeline initialization utilizing exclusively the Dynamic Rendering extension (no classic Render Passes).
- **World Management**: The world space is managed using a *Chunk System* structure, which allows for asynchronous streaming and real-time terrain modification.
- **Geometry Optimization**: Advanced optimization using *Greedy Meshing* and inter-chunk *Face Culling* algorithms, reducing the number of rendered vertices by over 90%.
- **VRAM Memory Management**: Efficient lifecycle management of graphics memory using the Vulkan Memory Allocator (VMA) library.
- **6DoF Camera**: Virtual camera support with full freedom of movement (6 Degrees of Freedom) and matrix transformations (Model-View-Projection).
- **Multithreading**: Utilization of multi-threaded mesh generation (meshing) for smooth navigation in a dynamically changing environment.
- **Diagnostics**: Error reporting based on Vulkan API validation layers.

## 🛠️ Technologies & Libraries

- **Language**: C++20
- **Graphics**: Vulkan 1.3
- **Window Management**: GLFW
- **User Interface (UI)**: Dear ImGui
- **Memory Management**: VMA (Vulkan Memory Allocator)
- **Mathematics**: GLM (OpenGL Mathematics)
- **Shader Compiler**: glslc (part of Vulkan SDK)
- **Build System**: CMake (minimum 3.20)

## 📂 Project Structure

- `assets/` - Engine resources (textures, models, etc.)
- `shaders/` - Shader source code (Vertex & Fragment in GLSL)
- `src/` - Main engine source code in C++
   - `core/` - Main engine components (e.g., lifecycle management)
   - `input/` - Input handling (keyboard, mouse)
   - `renderer/` - Code directly related to the Vulkan API (graphics pipeline, buffers)
   - `utils/` - Utility functions
   - `world/` - Voxel management, chunk system, mesh generation
- `vendor/` - External libraries (e.g., GLFW, ImGui, VMA)

## ⚙️ System Requirements & Building

### 📋 Requirements:
- A compiler supporting the **C++20** standard (e.g., GCC, Clang, MSVC)
- **CMake** (version 3.20 or newer)
- **Vulkan SDK** (version 1.3+) - installation is required to correctly build the project (including access to the `glslc` tool in the system PATH variable).

### 🏗️ Building:

The engine uses **CMake** as its build system.

1. Ensure you have the **Vulkan SDK** installed and configured in your system environment.
2. Clone the repository:
   ```bash
   git clone https://github.com/Siwyyy/VoxelEngine.git
   cd VoxelEngine
   ```
3. Generate the build files:
   ```bash
   cmake -B build
   ```
4. Build the project (Release mode is highly recommended for unlocking the true performance potential):
   ```bash
   cmake --build build --config Release
   ```
5. Run the engine:
   ```bash
   .\build\Release\VoxelEngine.exe
   ```

## 🎮 Controls

- **W, A, S, D** - Movement
- **Mouse** - Look around
- **Left Shift** - Move downwards
- **Space** - Move upwards
- **TAB** - Free the mouse cursor / Interact with debug menus
- **ESC** - Exit

## 📝 License

This project is open-source and available under the MIT License.