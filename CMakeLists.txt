cmake_minimum_required(VERSION 3.24)

project(pong VERSION 0.1 LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Release)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# SDL2
FetchContent_Declare(
  SDL2
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG        4761467b2e8cc7db3d6bc98747daca0051858f09 # release-2.28.1
  GIT_PROGRESS   TRUE
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL2)
set(SDL2_LIBRARIES SDL2 SDL2main)

# SDL2_gfx
FetchContent_Declare(
  SDL2_gfx
  URL https://sourceforge.net/projects/sdl2gfx/files/SDL2_gfx-1.0.4.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Populate(SDL2_gfx)
file(GLOB SDL2_gfx_srcs ${sdl2_gfx_SOURCE_DIR}/*.c)
add_library(SDL2_gfx EXCLUDE_FROM_ALL ${SDL2_gfx_srcs})
target_include_directories(SDL2_gfx PUBLIC ${sdl2_gfx_SOURCE_DIR})
target_link_libraries(SDL2_gfx ${SDL2_LIBRARIES})

# libm
find_library(MATH_LIBRARY m)

# main
add_compile_options(-Wpedantic -Wall -Wextra)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
add_executable(main main.c)
target_link_libraries(main ${SDL2_LIBRARIES} SDL2_gfx ${MATH_LIBRARY})
