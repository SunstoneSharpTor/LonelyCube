cmake_minimum_required(VERSION 3.27.4)
project(Minecraft VERSION 0.8.2 LANGUAGES CXX)

set(OSBitness 64)
set(FullOutputDir "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${FullOutputDir}/static_libs")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${FullOutputDir}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${FullOutputDir}")

include_directories(.)

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${GLEW_INCLUDE_DIRS})

set(SOURCE_FILES
    camera.cpp
    chunk.cpp
    indexBuffer.cpp
    main.cpp
    player.cpp
    random.cpp
    renderer.cpp
    shader.cpp
    stb_image.cpp
    texture.cpp
    vertexArray.cpp
    vertexBuffer.cpp
    world.cpp)

add_executable(Minecraft ${SOURCE_FILES})

target_link_libraries(Minecraft ${SDL2_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})