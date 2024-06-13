
include(FetchContent)

FetchContent_Declare(
  vulkan
  GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
  GIT_TAG        5677bafb820e476441e9e1f745371b72133407d3 # v1.3.285
)

FetchContent_MakeAvailable(vulkan)

set(Sources "src/layer.cpp" "src/renderer.cpp" "src/proxy.cpp" "src/vulkan/vkloader.hpp" "src/generic/KonideSceneLayer.cpp")

add_library(konide ${Sources})

target_link_libraries(konide Vulkan-Headers)

target_include_directories(konide PRIVATE "include/")

set_property(TARGET konide PROPERTY CXX_STANDARD 17)