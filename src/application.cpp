#include "application.hpp"
#include "utils.hpp"

#include <SDL3/SDL.h>
#include <vector>
#define VOLK_IMPLEMENTATION
#include <Volk/volk.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <iostream>

void Application::showError(const std::string &error_message) const {
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_message.c_str(),
                           window);
}

bool Application::initialize() {
  SDL_InitSubSystem(SDL_INIT_VIDEO);
  window = SDL_CreateWindow("Salkantkay", width, height,
                            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  if (!window) {
    std::string erro = "Erro ao criar janela: ";
    erro += SDL_GetError();
    showError(erro);
    return false;
  }

  if (!initializeVulkan()) {
    return false;
  }

  return true;
}

void Application::shutdown() {
  //  Wait in case resources are in use
  vkDeviceWaitIdle(device);

  //  Frame / sync object cleanup
  if (timeline_semaphore) {
    vkDestroySemaphore(device, timeline_semaphore, nullptr);
  }
  for (auto &res : frame_resources) {
    vkDestroySemaphore(device, res.image_acquired_semaphore, nullptr);
    vkDestroyCommandPool(device, res.command_pool, nullptr);
  }

  //  Pipeline cleanup
  if (pipeline_layout) {
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
  }

  if (pipeline) {
    vkDestroyPipeline(device, pipeline, nullptr);
  }

  //  Cleanup shaders
  if (vert_shader) {
    vkDestroyShaderModule(device, vert_shader, nullptr);
  }

  if (frag_shader) {
    vkDestroyShaderModule(device, frag_shader, nullptr);
  }

  //  Cleanup Swapchain
  destroySwapchain();

  //  VMA
  if (vma_allocator) {
    vmaDestroyAllocator(vma_allocator);
  }

  //  cleanup vulkan
  if (surface) {
    vkDestroySurfaceKHR(vulkan_instance, surface, nullptr);
  }
  if (device) {
    vkDestroyDevice(device, nullptr);
  }
  if (vulkan_instance) {
    vkDestroyInstance(vulkan_instance, nullptr);
  }
  volkFinalize();

  if (window) {
    SDL_DestroyWindow(window);
  }
  SDL_Quit();
}

void Application::run() {
  running = true;
  while (running) {
    SDL_Event event{0};
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
        break;
      } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        width = event.window.data1;
        height = event.window.data2;
        break;
      }
    }

    render();
  };
}

bool Application::initializeVulkan() {
  if (!createVulkanInstance()) {
    showError("Falha ao iniciar uma instancia Vulkan.");
    return false;
  }
  if (!createSurface()) {
    showError("Falha ao criar janela.");
    return false;
  }
  if (physical_device = findPhysicalDevice(); !physical_device) {
    showError("Falha ao encontrar um dispositivo fisico apropriado.");
    return false;
  }
  if (!findGraphicsQueue()) {
    showError("Falha ao encontrar uma fila grafica compativel.");
    return false;
  }
  if (!createDevice(physical_device)) {
    showError("Nao foi possivel criar o dispositivo logico da GPU.");
    return false;
  }
  if (!initializeVMA()) {
    showError("Nao foi possivel criar um alocador de memoria Vulkan.");
    return false;
  }
  if (!createSwapchain(width, height)) {
    showError("Nao foi possivel criar a Swapchain.");
    return false;
  }

  if (!createShaders()) {
    showError("Falha na criacao dos modulos do shaders.");
    return false;
  }

  if (pipeline = createGraphicsPipeline(); !pipeline) {
    showError("Nao foi possivel inicializar a pipeline grafica.");
    return false;
  }
  if (!createSyncResources()) {
    showError("Nao foi possivel sincronizar os recursos relacionados.");
    return false;
  }

  if (!createCommandBuffers()) {
    showError("Nao foi possivel criar os objetos do command buffer.");
    return false;
  }

  return true;
}

bool Application::createVulkanInstance() {
  //  Initialize Volk and load Vk function pointers.
  if (volkInitialize() != VK_SUCCESS) {
    showError("Falha na inicializacao do Volk.");
    return false;
  }

  //  Create the Vulkan Application Instance
  VkApplicationInfo app_info{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "Salkantkay!",
      .apiVersion = vulkan_version,
  };

  uint32_t inst_ext_count{0};
  const char *const *extensions{
      SDL_Vulkan_GetInstanceExtensions(&inst_ext_count)};

  std::vector<const char *> requested_layers{"VK_LAYER_KHRONOS_validation"};

  VkInstanceCreateInfo instance_create_info{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
      .enabledLayerCount = static_cast<uint32_t>(requested_layers.size()),
      .ppEnabledLayerNames = requested_layers.data(),
      .enabledExtensionCount = inst_ext_count,
      .ppEnabledExtensionNames = extensions,
  };

  if (vkCreateInstance(&instance_create_info, nullptr, &vulkan_instance) !=
      VK_SUCCESS) {
    return false;
  }

  volkLoadInstance(vulkan_instance);
  return true;
}

bool Application::createSurface() {
  if (!SDL_Vulkan_CreateSurface(window, vulkan_instance, nullptr, &surface)) {
    return false;
  }
  return true;
}

VkPhysicalDevice Application::findPhysicalDevice() {
  uint32_t physical_device_count{0};
  vkEnumeratePhysicalDevices(vulkan_instance, &physical_device_count, nullptr);
  std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
  vkEnumeratePhysicalDevices(vulkan_instance, &physical_device_count,
                             physical_devices.data());
  VkPhysicalDevice physical_device{nullptr};
  if (physical_device_count) {
    physical_device = physical_devices[0];
    for (auto &pDev : physical_devices) {
      VkPhysicalDeviceProperties props{};
      vkGetPhysicalDeviceProperties(pDev, &props);
      if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        physical_device = pDev;
        break;
      }
    }
  }

  uint32_t format_count{0};
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count,
                                       nullptr);
  std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count,
                                       surface_formats.data());

  bool format_supported{false};
  for (const VkSurfaceFormatKHR &surface_format : surface_formats) {
    if (surface_format.format == swapchain_format) {
      format_supported = true;
      break;
    }
  }
  if (!format_supported) {
    showError("Formato de swapchain requerida não está disponivel.");
    return nullptr;
  }

  return physical_device;
}

bool Application::findGraphicsQueue() {
  uint32_t queue_fam_count{0};
  vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queue_fam_count,
                                            nullptr);
  std::vector<VkQueueFamilyProperties2> queue_fam_props(
      queue_fam_count, {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
  vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queue_fam_count,
                                            queue_fam_props.data());

  for (int current_fam_idx{0}; current_fam_idx < queue_fam_props.size();
       current_fam_idx++) {
    VkBool32 has_present_support{false};
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, current_fam_idx,
                                         surface, &has_present_support);

    const auto &props{queue_fam_props[current_fam_idx]};
    if (props.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
        has_present_support) {
      gfx_queue_fam_idx = current_fam_idx;
      return true;
    }
  }
  return false;
}

bool Application::createDevice(VkPhysicalDevice physical_device) {
  float queue_priority{1.0f};
  std::vector<uint32_t> queue_families{gfx_queue_fam_idx};

  VkDeviceQueueCreateInfo gfx_queue_info{
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = gfx_queue_fam_idx,
      .queueCount = 1,
      .pQueuePriorities = &queue_priority};

  VkPhysicalDeviceVulkan14Features supported_features_14{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
      .pNext = nullptr};
  VkPhysicalDeviceVulkan13Features supported_features_13{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &supported_features_14};
  VkPhysicalDeviceVulkan12Features supported_features_12{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .pNext = &supported_features_13};
  VkPhysicalDeviceFeatures2 supported_features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &supported_features_12};
  vkGetPhysicalDeviceFeatures2(physical_device, &supported_features);

  if (!supported_features_13.dynamicRendering ||
      !supported_features_13.synchronization2 ||
      !supported_features_12.timelineSemaphore) {
    showError("Dispositivo fisico nao cumpre com os requerimentos basicos.");
    return false;
  }

  VkPhysicalDeviceVulkan14Features features_14{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
      .pNext = nullptr,
  };
  VkPhysicalDeviceVulkan13Features features_13{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &features_14,
      .synchronization2 = VK_TRUE,
      .dynamicRendering = VK_TRUE};
  VkPhysicalDeviceVulkan12Features features_12{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .pNext = &features_13,
      .timelineSemaphore = VK_TRUE};
  VkPhysicalDeviceFeatures2 features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
      .pNext = &features_12};

  const std::vector<const char *> device_extensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo dev_create_info{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &features,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &gfx_queue_info,
      .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
      .ppEnabledExtensionNames = device_extensions.data(),
      .pEnabledFeatures = nullptr};

  if (vkCreateDevice(physical_device, &dev_create_info, nullptr, &device) !=
      VK_SUCCESS) {
    return false;
  }

  vkGetDeviceQueue(device, gfx_queue_fam_idx, 0, &gfx_queue);
  if (!gfx_queue) {
    showError("Nao foi possivel encontrar a fila grafica");
    return false;
  }

  return true;
}

bool Application::initializeVMA() {
  VmaVulkanFunctions vma_function_info{};
  VmaAllocatorCreateInfo vma_allocator_info{
      .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
      .physicalDevice = physical_device,
      .device = device,
      .pVulkanFunctions = &vma_function_info,
      .instance = vulkan_instance,
      .vulkanApiVersion = vulkan_version};

  vmaImportVulkanFunctionsFromVolk(&vma_allocator_info, &vma_function_info);

  if (vmaCreateAllocator(&vma_allocator_info, &vma_allocator) != VK_SUCCESS) {
    return false;
  }
  return true;
}

bool Application::createSwapchain(uint32_t width, uint32_t height) {
  swapchain_width = width;
  swapchain_height = height;

  VkSurfaceCapabilitiesKHR surface_capabilities{};
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
          physical_device, surface, &surface_capabilities) != VK_SUCCESS) {
    showError("Nao pode buscar pelas capacidades da superficie");
    return false;
  }

  VkSwapchainCreateInfoKHR swapchain_create_info{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = surface_capabilities.minImageCount,
      .imageFormat = swapchain_format,
      .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
      .imageExtent{.width = swapchain_width, .height = swapchain_height},
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_FIFO_KHR};

  if (vkCreateSwapchainKHR(device, &swapchain_create_info, nullptr,
                           &swapchain) != VK_SUCCESS) {
    showError("Erro ao criar swapchain");
    return false;
  }

  uint32_t image_count{0};
  vkGetSwapchainImagesKHR(device, swapchain, &image_count, nullptr);
  swapchain_images.resize(image_count);
  vkGetSwapchainImagesKHR(device, swapchain, &image_count,
                          swapchain_images.data());
  swapchain_image_views.resize(image_count);

  for (size_t i{0}; i < swapchain_images.size(); ++i) {
    VkImageViewCreateInfo img_view_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = swapchain_images[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapchain_format,
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                          .levelCount = 1,
                          .layerCount = 1}};

    if (vkCreateImageView(device, &img_view_info, nullptr,
                          &swapchain_image_views[i]) != VK_SUCCESS) {
      showError("Error creating swapchain image views");
      return false;
    }
  }

  render_complete_semaphores.resize(swapchain_images.size());
  for (VkSemaphore &semaphore : render_complete_semaphores) {
    VkSemaphoreCreateInfo semaphore_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    if (vkCreateSemaphore(device, &semaphore_info, nullptr, &semaphore) !=
        VK_SUCCESS) {
      showError("Erro de criacao do semaforo de renderizacao");
      return false;
    }
  }

  VkImageCreateInfo depth_create_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = depth_format,
      .extent{.width = swapchain_width, .height = swapchain_height, .depth = 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

  VmaAllocationCreateInfo allocation_info{
      .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO};
  if (vmaCreateImage(vma_allocator, &depth_create_info, &allocation_info,
                     &depth_image, &depth_image_allocation,
                     nullptr) != VK_SUCCESS) {
    showError("Erro ao alocar a profundidade da imagem.");
    return false;
  }

  VkImageViewCreateInfo depth_image_view_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = depth_image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = depth_format,
      .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                        .levelCount = 1,
                        .layerCount = 1}};
  if (vkCreateImageView(device, &depth_image_view_info, nullptr,
                        &depth_image_view) != VK_SUCCESS) {
    showError("falha ao criar a visualizacao da profundidade da imagem");
    return false;
  }

  return true;
}

void Application::destroySwapchain() {
  for (VkImageView swapchain_image_view : swapchain_image_views) {
    vkDestroyImageView(device, swapchain_image_view, nullptr);
  }
  swapchain_image_views.clear();

  for (VkSemaphore &semaphore : render_complete_semaphores) {
    vkDestroySemaphore(device, semaphore, nullptr);
  }
  render_complete_semaphores.clear();

  if (swapchain) {
    vkDestroySwapchainKHR(device, swapchain, nullptr);
    swapchain = nullptr;
  }

  if (depth_image_view) {
    vkDestroyImageView(device, depth_image_view, nullptr);
    vmaDestroyImage(vma_allocator, depth_image, depth_image_allocation);
    depth_image_view = nullptr;
  }
}

VkShaderModule
Application::createShaderModule(const std::string &file_name) const {
  const std::string shader_path{"src/shaders/" + file_name + ".spv"};
  const std::vector<uint32_t> spv{readBinaryFile(shader_path)};
  if (spv.empty()) {
    showError("O spv n existe no caminho: " + shader_path);
    return nullptr;
  }

  VkShaderModuleCreateInfo module_create_info{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = spv.size() * sizeof(uint32_t),
      .pCode = spv.data()};

  VkShaderModule shader_module{nullptr};
  vkCreateShaderModule(device, &module_create_info, nullptr, &shader_module);
  return shader_module;
}

bool Application::createShaders() {
  if (vert_shader = createShaderModule("shader.vert"); !vert_shader) {
    return false;
  }
  if (frag_shader = createShaderModule("shader.frag"); !frag_shader) {
    return false;
  }
  return true;
}

VkPipeline Application::createGraphicsPipeline() {
  VkPipelineLayoutCreateInfo pipeline_layout_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,
      .pushConstantRangeCount = 0};
  if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr,
                             &pipeline_layout) != VK_SUCCESS) {
    showError("Impossibilitado de criar o layout da pipeline");
    return nullptr;
  }

  const char *entry_point = "main";
  std::vector<VkPipelineShaderStageCreateInfo> shader_stages{
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .module = vert_shader,
       .pName = entry_point},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
       .module = frag_shader,
       .pName = entry_point}};

  VkPipelineVertexInputStateCreateInfo vert_input_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

  VkPipelineInputAssemblyStateCreateInfo input_assembly_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};

  VkPipelineDepthStencilStateCreateInfo depth_stencil_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS,
      .stencilTestEnable = VK_FALSE};

  VkPipelineViewportStateCreateInfo viewport_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = nullptr,
      .scissorCount = 1,
      .pScissors = nullptr};

  VkPipelineRasterizationStateCreateInfo raster_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f};

  VkPipelineMultisampleStateCreateInfo multi_sample_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};

  VkPipelineColorBlendAttachmentState attach_state{
      .blendEnable = VK_FALSE,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
  VkPipelineColorBlendStateCreateInfo blend_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &attach_state};

  std::vector<VkDynamicState> dynamic_state{VK_DYNAMIC_STATE_VIEWPORT,
                                            VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamic_state_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = static_cast<uint32_t>(dynamic_state.size()),
      .pDynamicStates = dynamic_state.data()};

  VkPipelineRenderingCreateInfo render_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &swapchain_format,
      .depthAttachmentFormat = depth_format};

  VkGraphicsPipelineCreateInfo pipeline_info{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &render_info,
      .stageCount = static_cast<uint32_t>(shader_stages.size()),
      .pStages = shader_stages.data(),
      .pVertexInputState = &vert_input_info,
      .pInputAssemblyState = &input_assembly_info,
      .pViewportState = &viewport_info,
      .pRasterizationState = &raster_info,
      .pMultisampleState = &multi_sample_info,
      .pDepthStencilState = &depth_stencil_info,
      .pColorBlendState = &blend_info,
      .pDynamicState = &dynamic_state_info,
      .layout = pipeline_layout,
      .renderPass = VK_NULL_HANDLE};

  if (vkCreateGraphicsPipelines(device, nullptr, 1, &pipeline_info, nullptr,
                                &pipeline) != VK_SUCCESS) {
    showError("Erro ao criar a pipeline.");
    return nullptr;
  }
  return pipeline;
}

bool Application::createSyncResources() {
  VkSemaphoreTypeCreateInfo semaphore_type_info{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
      .initialValue = max_frames_in_flight};
  VkSemaphoreCreateInfo semaphore_info{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &semaphore_type_info};
  if (vkCreateSemaphore(device, &semaphore_info, nullptr,
                        &timeline_semaphore) != VK_SUCCESS) {
    showError("Impossivel criar semaforo timeline.");
    return false;
  }

  for (FrameResources &res : frame_resources) {
    VkSemaphoreCreateInfo semaphore_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    if (vkCreateSemaphore(device, &semaphore_info, nullptr,
                          &res.image_acquired_semaphore) != VK_SUCCESS) {
      showError("Falha ao criar semaforo com aquisicao de imagem por frames.");
      return false;
    }
  }

  return true;
}

bool Application::createCommandBuffers() {
  for (FrameResources &res : frame_resources) {
    VkCommandPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = gfx_queue_fam_idx};
    if (vkCreateCommandPool(device, &pool_info, nullptr, &res.command_pool) !=
        VK_SUCCESS) {
      showError("Nao foi possivel criar a pool de command buffer");
      return false;
    }

    VkCommandBufferAllocateInfo cmd_allocate_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = res.command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1};
    if (vkAllocateCommandBuffers(device, &cmd_allocate_info,
                                 &res.command_buffer) != VK_SUCCESS) {
      showError("Nao foi possivel alocar command buffers.");
      return false;
    }
  }

  return true;
}

void Application::render() {
  if (require_swapchain_in_recreate) {
    vkDeviceWaitIdle(device);
    destroySwapchain();
    createSwapchain(width, height);
    require_swapchain_in_recreate = false;
  }

  const uint32_t frame_resource_index = frame_index++ % max_frames_in_flight;
  const uint64_t signal_value = next_signal_value++;
  const uint64_t wait_value = signal_value - max_frames_in_flight;

  VkSemaphoreWaitInfo wait_info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
                                .semaphoreCount = 1,
                                .pSemaphores = &timeline_semaphore,
                                .pValues = &wait_value};
  vkWaitSemaphores(device, &wait_info, UINT64_MAX);

  FrameResources &res = frame_resources[frame_resource_index];
  vkResetCommandPool(device, res.command_pool, 0);

  VkSemaphore image_acquire_semaphore =
      frame_resources[frame_resource_index].image_acquired_semaphore;

  uint32_t image_index = 0;
  VkResult acquire_result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
                                                  image_acquire_semaphore,
                                                  VK_NULL_HANDLE, &image_index);
  if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
    require_swapchain_in_recreate = true;
    return;
  } else if (acquire_result == VK_SUBOPTIMAL_KHR) {
    require_swapchain_in_recreate = true;
  }

  VkCommandBufferBeginInfo cmd_begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  vkBeginCommandBuffer(res.command_buffer, &cmd_begin_info);

  std::vector<VkImageMemoryBarrier2> layout_barriers{
      {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
       .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
       .srcAccessMask = 0,
       .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
       .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
       .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
       .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
       .image = swapchain_images[image_index],
       .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                         .baseMipLevel = 0,
                         .levelCount = 1,
                         .baseArrayLayer = 0,
                         .layerCount = 1

       }},
      {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
       .srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
       .srcAccessMask = 0,
       .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                       VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
       .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
       .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
       .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
       .image = depth_image,
       .subresourceRange{
           .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
           .baseMipLevel = 0,
           .levelCount = 1,
           .baseArrayLayer = 0,
           .layerCount = 1,
       }}};
  VkDependencyInfo dependency_info{
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = static_cast<uint32_t>(layout_barriers.size()),
      .pImageMemoryBarriers = layout_barriers.data()};
  vkCmdPipelineBarrier2(res.command_buffer, &dependency_info);

  VkRenderingAttachmentInfo color_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = swapchain_image_views[image_index],
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue{.color{{0.01f, 0.01f, 0.01f, 1}}}};
  VkRenderingAttachmentInfo depth_attachment_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = depth_image_view,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = {.depthStencil{1.0f, 0}}};
  VkRenderingInfo rendering_info{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {.offset{.x = 0, .y = 0},
                     .extent{.width = swapchain_width,
                             .height = swapchain_height}},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_info,
      .pDepthAttachment = &depth_attachment_info};

  vkCmdBeginRendering(res.command_buffer, &rendering_info);
  {
    VkViewport viewport{.x = 0,
                        .y = 0,
                        .width = static_cast<float>(swapchain_width),
                        .height = static_cast<float>(swapchain_height)};
    vkCmdSetViewport(res.command_buffer, 0, 1, &viewport);

    VkRect2D scissor{
        .offset{.x = 0, .y = 0},
        .extent{.width = swapchain_width, .height = swapchain_height}};
    vkCmdSetScissor(res.command_buffer, 0, 1, &scissor);

    vkCmdBindPipeline(res.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipeline);
    vkCmdDraw(res.command_buffer, 3, 1, 0, 0);
  }
  vkCmdEndRendering(res.command_buffer);

  VkImageMemoryBarrier2 present_layout_barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
      .dstAccessMask = 0,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .image = swapchain_images[image_index],
      .subresourceRange{
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      }};
  VkDependencyInfo present_dependency_info{
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &present_layout_barrier};
  vkCmdPipelineBarrier2(res.command_buffer, &present_dependency_info);
  vkEndCommandBuffer(res.command_buffer);

  VkSemaphoreSubmitInfo image_acquire_wait_info{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = image_acquire_semaphore,
      .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT};
  std::vector<VkSemaphoreSubmitInfo> present_signal{
      {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
       .semaphore = render_complete_semaphores[image_index],
       .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT},
      {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
       .semaphore = timeline_semaphore,
       .value = signal_value,
       .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT}};
  VkCommandBufferSubmitInfo command_submit_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .commandBuffer = res.command_buffer};
  VkSubmitInfo2 submit_info{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                            .waitSemaphoreInfoCount = 1,
                            .pWaitSemaphoreInfos = &image_acquire_wait_info,
                            .commandBufferInfoCount = 1,
                            .pCommandBufferInfos = &command_submit_info,
                            .signalSemaphoreInfoCount =
                                static_cast<uint32_t>(present_signal.size()),
                            .pSignalSemaphoreInfos = present_signal.data()};
  vkQueueSubmit2(gfx_queue, 1, &submit_info, VK_NULL_HANDLE);

  VkPresentInfoKHR present_info{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                                .waitSemaphoreCount = 1,
                                .pWaitSemaphores =
                                    &render_complete_semaphores[image_index],
                                .swapchainCount = 1,
                                .pSwapchains = &swapchain,
                                .pImageIndices = &image_index,
                                .pResults = nullptr};

  vkQueuePresentKHR(gfx_queue, &present_info);
}
