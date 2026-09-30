cmake_minimum_required(VERSION 3.15)
project(mygit VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DMINIZ_NO_ARCHIVE_APIS)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/utils.cpp
    src/repository.cpp
    src/object.cpp
    src/tree.cpp
    src/commit.cpp
    src/branch.cpp
    src/index.cpp
    src/status.cpp
    src/diff.cpp
    src/ignore.cpp
    src/tag.cpp
    src/miniz.c
)

add_executable(mygit ${SOURCES})