cmake_minimum_required(VERSION 3.25.1)
project(LonelyCube VERSION 0.0.0 LANGUAGES C CXX)

enable_testing()

add_subdirectory(lib/enet-1.3.18)
add_subdirectory(lib/glfw-3.4)

set(OSBitness 64)
set(FullOutputDir "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${FullOutputDir}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${FullOutputDir}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (CMAKE_CXX_STANDARD 20)

include_directories(. ./src ./lib ./lib/enet-1.3.18/include ./lib/glfw-3.4/include)

set(CLIENT_SOURCE_FILES
    lib/glad/glad.c
    src/client/client.cpp
    src/client/clientNetworking.cpp
    src/client/clientPlayer.cpp
    src/client/clientWorld.cpp
    src/client/graphics/bloom.cpp
    src/client/graphics/camera.cpp
    src/client/graphics/computeShader.cpp
    src/client/graphics/indexBuffer.cpp
    src/client/graphics/luminance.cpp
    src/client/graphics/meshBuilder.cpp
    src/client/graphics/renderer.cpp
    src/client/graphics/shader.cpp
    src/client/graphics/stb_image.cpp
    src/client/graphics/texture.cpp
    src/client/graphics/vertexArray.cpp
    src/client/graphics/vertexBuffer.cpp
    src/client/gui/font.cpp
    src/client/logicThread.cpp
    src/client/renderThread.cpp
    src/core/chunk.cpp
    src/core/chunkManager.cpp
    src/core/compression.cpp
    src/core/config.cpp
    src/core/entities/ECS.cpp
    src/core/entities/entityManager.cpp
    src/core/entities/physicsEngine.cpp
    src/core/entities/components/meshComponent.cpp
    src/core/entities/components/transformComponent.cpp
    src/core/lighting.cpp
    src/core/serverPlayer.cpp
    src/core/random.cpp
    src/core/resourcePack.cpp
    src/core/resourceMonitor.cpp
    src/core/terrainGen.cpp
    src/core/threadManager.cpp
    src/core/utils/iVec3.cpp)

add_executable(client ${CLIENT_SOURCE_FILES})
target_link_libraries(client enet glfw)
target_precompile_headers(client PRIVATE
$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/src/core/pch.h>)

set(SERVER_SOURCE_FILES
    src/core/chunk.cpp
    src/core/chunkManager.cpp
    src/core/compression.cpp
    src/core/entities/ECS.cpp
    src/core/entities/entityManager.cpp
    src/core/entities/physicsEngine.cpp
    src/core/entities/components/meshComponent.cpp
    src/core/entities/components/transformComponent.cpp
    src/core/lighting.cpp
    src/core/serverPlayer.cpp
    src/core/random.cpp
    src/core/resourceMonitor.cpp
    src/core/resourcePack.cpp
    src/core/terrainGen.cpp
    src/core/threadManager.cpp
    src/core/utils/iVec3.cpp
    src/server/server.cpp
    src/server/serverNetworking.cpp)

add_executable(server ${SERVER_SOURCE_FILES})
target_link_libraries(server enet)
target_precompile_headers(server REUSE_FROM client)

if (MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    target_link_options(client PRIVATE -mconsole)
    target_link_libraries(client winmm ws2_32)
    target_link_libraries(server winmm ws2_32)
endif (MINGW)

# TODO: Change this to check for opengl/gles3 support when raspberry pi cmake version gets updated
if (${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "aarch64")
    add_compile_definitions(GLES3)
endif()


add_custom_command(TARGET client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/res
            ${FullOutputDir}/res)

add_subdirectory(tests)
