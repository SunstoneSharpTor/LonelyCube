cmake_minimum_required(VERSION 3.25.1)
project(LonelyCube VERSION 0.0.0 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 20)

set(OSBitness 64)
set(FullOutputDir "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${FullOutputDir}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${FullOutputDir}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

include(CheckIPOSupported)
check_ipo_supported(RESULT IPOSupported OUTPUT error)
if(IPOSupported)
    # set_property(TARGET client PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif()

include(FetchContent)

FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG        v1.3.18
)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        f58f558c120e9b32c217290b80bad1a0729fbb2c  # 26/05/2025
)
FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG        v0.12.1
)
FetchContent_Declare(
    volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG        1.4.304
)
FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG        v3.3.0
)
FetchContent_MakeAvailable(enet glfw glm stb tracy volk VulkanMemoryAllocator)

add_subdirectory(tests)

set(CLIENT_SOURCE_FILES
    src/client/applicationState.cpp
    src/client/client.cpp
    src/client/clientNetworking.cpp
    src/client/clientPlayer.cpp
    src/client/clientWorld.cpp
    src/client/game.cpp
    src/client/graphics/bloom.cpp
    src/client/graphics/camera.cpp
    src/client/graphics/entityMeshManager.cpp
    src/client/graphics/autoExposure.cpp
    src/client/graphics/meshBuilder.cpp
    src/client/graphics/renderer.cpp
    src/client/graphics/vulkan/descriptors.cpp
    src/client/graphics/vulkan/pipelines.cpp
    src/client/graphics/vulkan/shaders.cpp
    src/client/graphics/vulkan/vulkanEngine.cpp
    src/client/graphics/vulkan/images.cpp
    src/client/gui/font.cpp
    src/client/input.cpp
    src/client/gui/menu.cpp
    src/client/gui/menuRenderer.cpp
    src/client/gui/pauseMenu.cpp
    src/client/gui/startMenu.cpp
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
    src/core/log.cpp
    src/core/serverPlayer.cpp
    src/core/random.cpp
    src/core/resourcePack.cpp
    src/core/resourceMonitor.cpp
    src/core/terrainGen.cpp
    src/core/threadManager.cpp
    src/core/utils/iVec3.cpp)

add_executable(client ${CLIENT_SOURCE_FILES})
target_include_directories(client PRIVATE
    ./src
    ./lib
    ${enet_SOURCE_DIR}/include
    ${stb_SOURCE_DIR}
)
target_link_libraries(client PRIVATE
    enet
    glfw
    glm::glm
    GPUOpen::VulkanMemoryAllocator
    # Tracy::TracyClient
    volk
)

target_compile_definitions(client PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_LEFT_HANDED)
if (CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(client PRIVATE RELEASE)
endif()

target_precompile_headers(client PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_SOURCE_DIR}/src/core/pch.h>)

# target_compile_options(client PRIVATE -fsanitize=address)
# target_link_options(client PRIVATE -fsanitize=address)

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
    src/core/log.cpp
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
target_include_directories(server PRIVATE
    ./src
    ./lib
    ${enet_SOURCE_DIR}/include
)
target_link_libraries(server PRIVATE enet glm::glm) #Tracy::TracyClient)

target_compile_definitions(server PRIVATE GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_LEFT_HANDED)
if (CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(server PRIVATE RELEASE)
endif()

target_precompile_headers(server REUSE_FROM client)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    if (NOT ${CMAKE_BUILD_TYPE} MATCHES Release)
        target_link_options(client PRIVATE -mconsole)
    endif()
    target_link_libraries(client PRIVATE winmm ws2_32)
    target_link_libraries(server PRIVATE winmm ws2_32)
endif()


add_custom_command(TARGET client POST_BUILD
    # Copy the res folder to the bin directory
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/res
            ${FullOutputDir}/res

    # Copy compile_commands.json to the build directory
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/build/compile_commands.json
)
