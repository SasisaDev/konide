
include(FetchContent)

FetchContent_Declare(
  sdl
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG        release-2.30.3 # release-2.30.3
)

FetchContent_MakeAvailable(sdl)

add_executable(HelloTriangle WIN32 "main.cpp")

target_link_libraries(HelloTriangle debug konide SDL2)
target_include_directories(HelloTriangle PRIVATE "../../konide/include/")

set_property(TARGET HelloTriangle PROPERTY CXX_STANDARD 17)