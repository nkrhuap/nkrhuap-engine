A low-level, high-performance rendering architecture built from scratch in C++23 focused on modern GPU synchronization and zero-overhead memory management.
- **Dynamic Rendering Pipeline:** Eliminates legacy Vulkan render pass/framebuffer boilerplate via explicit dynamic rendering states and synchronization primitives.
- **Modern Synchronization:** Native integration of Vulkan 1.4 timeline semaphores for lock-free GPU-to-CPU execution flow and frame pacing control.
- **Explicit Memory Control:** Integrated Vulkan Memory Allocator (VMA) for custom host/device heap allocations, mapped buffers, and sub-allocation tracking.
- **Target Architecture:** Modern x86_64 toolchains using C++23 features, compiled via Clang and Ninja under a minimal zero-dependency build setup.
