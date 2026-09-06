#pragma once

#define VK_NO_PROTOTYPES
#include <SDL3/SDL_vulkan.h>
#include <array>
#include <shaderc/shaderc.hpp>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

struct SDL_Window;
struct VmaAllocator_T;
typedef struct VmaAllocator_T *VmaAllocator;
struct VmaAllocation_T;
typedef struct VmaAllocation_T *VmaAllocation;

struct FrameResources {
  VkCommandPool command_pool{nullptr};
  VkCommandBuffer command_buffer{nullptr};
  VkSemaphore image_acquired_semaphore{nullptr};
};

class Application {
  constexpr static uint32_t vulkan_version{VK_API_VERSION_1_4};
  constexpr static uint32_t max_frames_in_flight{2};
  constexpr static VkFormat swapchain_format{VK_FORMAT_B8G8R8A8_SRGB};
  constexpr static VkFormat depth_format{VK_FORMAT_D32_SFLOAT};

  SDL_Window *window{nullptr};
  uint32_t width{1280};
  uint32_t height{720};
  bool running{false};
  uint64_t frame_index{0};
  uint64_t next_signal_value{max_frames_in_flight + 1};

  //  Vulkan Core
  VkInstance vulkan_instance{nullptr};
  VkPhysicalDevice physical_device{nullptr};
  VkDevice device{nullptr};
  VkSurfaceKHR surface{nullptr};
  VmaAllocator vma_allocator{nullptr};

  //  Queue Related
  uint32_t gfx_queue_fam_idx{UINT32_MAX};
  VkQueue gfx_queue{nullptr};

  //  Swapchain Related
  VkSwapchainKHR swapchain{nullptr};
  std::vector<VkImage> swapchain_images;
  std::vector<VkImageView> swapchain_image_views;
  std::vector<VkSemaphore> render_complete_semaphores;
  bool require_swapchain_in_recreate{false};
  uint32_t swapchain_width{0};
  uint32_t swapchain_height{0};

  VkImage depth_image{nullptr};
  VkImageView depth_image_view{nullptr};
  VmaAllocation depth_image_allocation{nullptr};

  //  Graphics Pipeline Related
  VkPipelineLayout pipeline_layout{nullptr};
  VkPipeline pipeline{nullptr};

  // Shader Resources
  VkShaderModule vert_shader{nullptr};
  VkShaderModule frag_shader{nullptr};

  //  Frame and synchronization resources
  VkSemaphore timeline_semaphore{nullptr};
  std::array<FrameResources, max_frames_in_flight> frame_resources;

  void showError(const std::string &error_message) const;

  bool initializeVulkan();
  bool createVulkanInstance();
  bool createSurface();
  VkPhysicalDevice findPhysicalDevice();
  bool findGraphicsQueue();
  bool createDevice(VkPhysicalDevice physicalDevice);
  bool initializeVMA();
  bool createSwapchain(uint32_t width, uint32_t height);
  void destroySwapchain();
  VkShaderModule createShaderModule(const std::string &fileName,
                                    shaderc_shader_kind kind) const;
  bool createShaders();
  VkPipeline createGraphicsPipeline();
  bool createSyncResources();
  bool createCommandBuffers();
  void render();

public:
  bool initialize();
  void shutdown();
  void run();
};
