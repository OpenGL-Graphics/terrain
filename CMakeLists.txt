cmake_minimum_required(VERSION 3.10)
project(terrain)

# use C++17 (in case std::fs needed)
set(CMAKE_CXX_STANDARD 17)

# autocomplete with YCM & debug with gdb
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Release)
# set(CMAKE_BUILD_TYPE Debug)

# copy assets folder
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})

# glfw-window & opengl-utils (texture, shader, & framebuffer) libraries from submodules
add_subdirectory(glfw-window)
add_subdirectory(opengl-utils)

# executable
file(GLOB SRC
  "src/geometries/*.cpp"
  "src/render/*.cpp"
)

add_executable(main src/main.cpp ${SRC})
target_include_directories(main PUBLIC include)
target_link_libraries(main
  glfw_window
  opengl_utils
)
