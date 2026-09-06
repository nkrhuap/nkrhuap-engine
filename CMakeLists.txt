cmake_minimum_required(VERSION 3.14)
project(QueroMandioca)

find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED COMPONENTS volk shaderc_combined)
find_package(glm REQUIRED)

add_executable(vulkanapp
  "src/main.cpp"
  "src/application.hpp"
  "src/application.cpp"
  "src/utils.hpp"
  "src/utils.cpp"
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  target_compile_options(vulkanapp PRIVATE /std:c++latest)
endif()

target_link_libraries(vulkanapp PRIVATE
  Vulkan::Vulkan
  Vulkan::volk
  Vulkan::shaderc_combined
  SDL3::SDL3
  glm::glm
)
