cmake_minimum_required(VERSION 3.27.4)
project(Minecraft VERSION 0.8.2 LANGUAGES C CXX)

set(OSBitness 64)
set(FullOutputDir "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${FullOutputDir}/static_libs")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${FullOutputDir}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${FullOutputDir}")

include_directories(. ./src/core ./src/client ./lib)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

set(CLIENT_SOURCE_FILES
    lib/glad/glad.c
    src/core/camera.cpp
    src/core/chunk.cpp
    src/core/indexBuffer.cpp
    src/core/player.cpp
    src/core/random.cpp
    src/core/renderer.cpp
    src/core/shader.cpp
    src/core/stb_image.cpp
    src/core/texture.cpp
    src/core/vertexArray.cpp
    src/core/vertexBuffer.cpp
    src/core/world.cpp
    src/client/client.cpp)

add_executable(client ${CLIENT_SOURCE_FILES})
target_link_libraries(client ${SDL2_LIBRARIES})

set(SERVER_SOURCE_FILES
    src/server/server.cpp)

add_executable(server ${SERVER_SOURCE_FILES})

if (MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif(MINGW)