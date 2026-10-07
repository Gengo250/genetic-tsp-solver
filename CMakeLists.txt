cmake_minimum_required(VERSION 3.16)

project(genetic_tsp_solver LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Gera compile_commands.json para o clangd encontrar os headers.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Pega automaticamente qualquer .c novo dentro de src/.
file(GLOB SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")

add_executable(genetic_tsp_solver ${SOURCES})

target_include_directories(genetic_tsp_solver PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_compile_options(genetic_tsp_solver PRIVATE -Wall -Wextra)

# libm (math.h)
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
  target_link_libraries(genetic_tsp_solver PRIVATE ${MATH_LIBRARY})
endif()

# raylib (interface grafica)
find_library(RAYLIB_LIBRARY raylib REQUIRED)
target_link_libraries(genetic_tsp_solver PRIVATE ${RAYLIB_LIBRARY})
