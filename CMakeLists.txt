cmake_minimum_required(VERSION 3.10)
project(GitHeatMap)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Прямое указание путей для Homebrew на M1/M2
set(GIT2_INCLUDE_DIR "/opt/homebrew/include")
set(GIT2_LIBRARY "/opt/homebrew/lib/libgit2.dylib")

add_executable(GitHeatMap main.cpp)

target_include_directories(GitHeatMap PRIVATE ${GIT2_INCLUDE_DIR})
target_link_libraries(GitHeatMap PRIVATE ${GIT2_LIBRARY})