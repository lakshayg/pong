cmake_minimum_required(VERSION 3.11)

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
)
FetchContent_MakeAvailable(SDL2)
set(SDL2_LIBRARIES SDL2 SDL2main)

# main
add_compile_options(-Wpedantic -Wall -Wextra)
add_executable(main main.c)
target_link_libraries(main ${SDL2_LIBRARIES})
