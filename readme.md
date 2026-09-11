**nkrhuap-engine**

This project is a hardware-accelerated graphics application built with C++26, leveraging modern Vulkan features for efficient rendering. It initializes a resizable graphical window and manages a dedicated swapchain to present rendered frames directly to the user.

**Core Application APIs & Dependencies**

* **Vulkan API (1.4):** The primary graphics API used for backend rendering, specifically utilizing modern extensions like dynamic rendering, timeline semaphores, and Synchronization2 to minimize driver overhead.


* **SDL3 (Simple DirectMedia Layer):** Handles cross-platform window creation, Vulkan surface instantiation, and main loop event polling (such as window resizing and exit commands).


* **Vulkan Memory Allocator (VMA):** Simplifies the complex allocation of dedicated GPU memory for resources like the 32-bit depth buffer image.


* **Volk:** A Vulkan meta-loader used to dynamically load function pointers at runtime, avoiding static linking issues and improving performance.


* **GLM (OpenGL Mathematics):** Included as a dependency to handle complex vector, matrix, and projection math operations.


* **DirectX Shader Compiler (DXC) & Shaderc:** Integrated directly into the CMake build process to dynamically compile the raw HLSL vertex and fragment shaders into Vulkan-compatible SPIR-V byte code.



**Architecture Summary**

The application abstracts the verbose Vulkan setup into a centralized `Application` class. The initialization phase automatically locates a discrete GPU, sets up the graphics queues, and configures a `B8G8R8A8_SRGB` swapchain. Instead of legacy render pass objects, the render loop utilizes Vulkan's dynamic rendering (`vkCmdBeginRendering`) and submits hardcoded triangle draw commands with pipeline barriers for memory synchronization. Frames are managed concurrently using a maximum of two frames in flight, synchronized tightly via modern timeline semaphores to prevent GPU idling.

**Build Instructions**

* Ensure a C++26 compatible compiler is installed (such as Clang-cl or MSVC).


* Install the Vulkan SDK, which provides the necessary DXC executable for shader compilation.

* Configure the build using CMake and the vcpkg toolchain to automatically resolve and build dependencies like SDL3 and GLM.  
