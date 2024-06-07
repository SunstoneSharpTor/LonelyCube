cmake_minimum_required(VERSION 3.27.4)
project(Minecraft VERSION 0.8.2 LANGUAGES C CXX)

set(OSBitness 64)
set(FullOutputDir "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${FullOutputDir}/static_libs")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${FullOutputDir}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${FullOutputDir}")

find_package(SDL2 REQUIRED)

include_directories(. ./src ./lib ${SDL2_INCLUDE_DIRS} ./lib/enet-1.3.18/include)

set(CLIENT_SOURCE_FILES
    lib/glad/glad.c
    src/client/camera.cpp
    src/client/client.cpp
    src/client/indexBuffer.cpp
    src/client/renderer.cpp
    src/client/player.cpp
    src/client/shader.cpp
    src/client/stb_image.cpp
    src/client/texture.cpp
    src/client/vertexArray.cpp
    src/client/vertexBuffer.cpp
    src/client/world.cpp
    src/core/chunk.cpp
    src/core/random.cpp)

add_executable(client ${CLIENT_SOURCE_FILES})
target_link_libraries(client ${SDL2_LIBRARIES})

set(SERVER_SOURCE_FILES
    src/core/chunk.cpp
    src/core/random.cpp
    src/server/server.cpp
    src/server/world.cpp)

add_executable(server ${SERVER_SOURCE_FILES})

if (MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-static")

    target_link_libraries(client ${CMAKE_SOURCE_DIR}/lib/enet-1.3.18/enet64.lib
                                 winmm ws2_32)
    target_link_libraries(server ${CMAKE_SOURCE_DIR}/lib/enet-1.3.18/enet64.lib
                                 winmm ws2_32)
elseif (UNIX)
    target_link_libraries(client ${CMAKE_SOURCE_DIR}/lib/enet-1.3.18/libenet.a)
    target_link_libraries(server ${CMAKE_SOURCE_DIR}/lib/enet-1.3.18/libenet.a)
endif(MINGW)