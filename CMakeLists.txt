cmake_minimum_required(VERSION 3.15)
project(3D_renderer LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(dependencies/minifb)
add_executable(3D_renderer src/3D_renderer.cpp)

target_link_libraries(3D_renderer PRIVATE minifb)

add_custom_command(TARGET 3D_renderer POST_BUILD
  COMMAND $(CMAKE_COMMAND) -E copy $<TARGET_FILE:3D_renderer> $(CMAKE_SOURCE_DIR)
)
