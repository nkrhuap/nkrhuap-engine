cmake_minimum_required(VERSION 3.14)
project(nkrhuap)

find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED COMPONENTS volk shaderc_combined)
find_package(glm REQUIRED)

# INICIO - COMPILAR HLSL PARA SPIR-V
find_program(DXC_EXECUTABLE dxc HINTS "$ENV{VULKAN_SDK}/bin")

if(NOT DXC_EXECUTABLE)
  message(FATAL_ERROR "Compilador DXC não encontrado. Dá uma olhada no SDK do Vulkan.")
endif()

set(SHADERS
  src/shaders/shader.vert.hlsl
  src/shaders/shader.frag.hlsl
)

set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/src/shaders")
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

set(SPV_OUTPUTS "")

foreach(SHADER IN LISTS SHADERS)
  get_filename_component(FILE_NAME ${SHADER} NAME_WLE)

  get_filename_component(EXT ${SHADER} EXT)

  set(SPV_FILE "${SHADER_OUTPUT_DIR}/${FILE_NAME}.spv")

  if(${FILE_NAME} MATCHES ".vert$")
    set(PROFILE "vs_6_0")
  elseif(${FILE_NAME} MATCHES ".frag$")
    set(PROFILE "ps_6_0")
  endif()

  add_custom_command(
    OUTPUT ${SPV_FILE}
    COMMAND ${DXC_EXECUTABLE} -spirv -T ${PROFILE} -E main ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER} -Fo ${SPV_FILE}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
    COMMENT "Compilando ${FILE_NAME}"
  )

  list(APPEND SPV_OUTPUTS ${SPV_FILE})
endforeach()

add_custom_target(CompileShaders ALL DEPENDS ${SPV_OUTPUTS})
# FIM - COMPILAR HLSL PARA SPIR-V

add_executable(nkrhuap
  "src/main.cpp"
  "src/application.hpp"
  "src/application.cpp"
  "src/utils.hpp"
  "src/utils.cpp"
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  target_compile_options(nkrhuap PRIVATE /std:c++latest)
endif()

add_dependencies(nkrhuap CompileShaders)

target_link_libraries(nkrhuap PRIVATE
  Vulkan::Vulkan
  Vulkan::volk
  Vulkan::shaderc_combined
  SDL3::SDL3
  glm::glm
)
