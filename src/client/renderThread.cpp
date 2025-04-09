/*
  Lonely Cube, a voxel game
  Copyright (C) 2024-2025 Bertie Cartwright

  Lonely Cube is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Lonely Cube is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "client/renderThread.h"

#include <GLFW/glfw3.h>
#include <enet/enet.h>
#include <iterator>
#include "glm/fwd.hpp"
#include "glm/matrix.hpp"
#include "stb_image.h"

#include "core/pch.h"

#include "client/logicThread.h"
#include "client/clientNetworking.h"
#include "client/graphics/renderer.h"
#include "client/graphics/vulkan/vulkanEngine.h"
#include "client/graphics/camera.h"
#include "client/clientWorld.h"
#include "client/clientPlayer.h"
#include "core/config.h"
#include "core/constants.h"
#include "core/log.h"

#include "glm/glm.hpp"

namespace lonelycube::client {

static float calculateBrightness(const float* points, uint32_t numPoints, uint32_t time) {
    uint32_t preceedingPoint = numPoints * 2 - 2;
    uint32_t succeedingPoint = 0;
    if (time < points[numPoints * 2 - 2]) {
        int i = 0;
        while (i < numPoints * 2 && points[i] < time) {
            i += 2;
            preceedingPoint = i - 2;
            succeedingPoint = i;
        }
    }
    float preceedingTime = points[preceedingPoint];
    float succeedingTime = points[succeedingPoint];
    if (succeedingTime < preceedingTime) {
        float offset = (float)constants::DAY_LENGTH - preceedingTime;
        preceedingTime = 0;
        time = (time + (int)offset) % constants::DAY_LENGTH;
        succeedingTime += offset;
    }
    float frac = ((float)time - preceedingTime) / (succeedingTime - preceedingTime);

    return points[succeedingPoint + 1] * frac + points[preceedingPoint + 1] * (1.0f - frac);
}

static std::string testText;

void characterCallback(GLFWwindow* window, unsigned int codepoint)
{
    testText = testText + (char)codepoint;
}

void renderThread() {
    Config settings("res/settings.txt");

    bool multiplayer = settings.getMultiplayer();

    ClientNetworking networking;

    if (multiplayer) {
        if (!networking.establishConnection(settings.getServerIP(), settings.getRenderDistance())) {
            multiplayer = false;
        }
    }

    Renderer renderer(VK_SAMPLE_COUNT_4_BIT, 1.0f);

    uint32_t worldSeed = std::time(0);
    int playerSpawnPoint[3] = { 0, 200, 0 };
    ClientWorld mainWorld(
        settings.getRenderDistance(), worldSeed, !multiplayer, playerSpawnPoint,
        multiplayer ? networking.getPeer() : nullptr, networking.getMutex(), renderer
    );
    LOG("World Seed: " + std::to_string(worldSeed));
    ClientPlayer mainPlayer(
        playerSpawnPoint, &mainWorld, mainWorld.integratedServer.getResourcePack()
    );

    bool running = true;
    GLFWwindow* window;

    bool* chunkLoaderThreadsRunning = new bool[mainWorld.getNumChunkLoaderThreads()];
    std::fill(
        chunkLoaderThreadsRunning, chunkLoaderThreadsRunning + mainWorld.getNumChunkLoaderThreads(),
        true
    );

    LogicThread logicThread(
        mainWorld, chunkLoaderThreadsRunning, mainPlayer, networking, settings, multiplayer
    );
    std::thread logicWorker(&LogicThread::go, logicThread, std::ref(running));

    bool windowLastFocus = false;
    bool windowFullScreen = false;
    bool lastWindowFullScreen = false;
    bool lastLastWindowFullScreen = false;
    int windowRestoredSize[2];
    int windowRestoredPos[2];
    bool lastF11 = false;
    int windowDimensions[2];
    int smallScreenWindowDimensions[2];
    int smallScreenWindowPos[2];
    glfwGetWindowSize(
        renderer.getVulkanEngine().getWindow(), &windowDimensions[0], &windowDimensions[1]
    );
    const GLFWvidmode* displayMode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    mainWorld.updatePlayerPos(mainPlayer.cameraBlockPosition, &(mainPlayer.viewCamera.position[0]));

    double time;
    mainPlayer.processUserInput(
        renderer.getVulkanEngine().getWindow(), windowDimensions, &windowLastFocus, time,
        networking
    );
    mainWorld.doRenderThreadJobs();

    // glfwSetCharCallback(window, characterCallback);

    //set up game loop
    float exposure = 0.0;
    float toneMapTimeByDTs = 0.0;
    int FPS_CAP = std::numeric_limits<int>::max();
    double DT = 1.0 / FPS_CAP;
    uint64_t lastFrameRateFrames = 0;
    auto start = std::chrono::steady_clock::now();
    auto end = start;
    time = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000;
    double frameStart = time - DT;
    float lastFrameRateTime = frameStart + DT;
    while (running) {
        glfwPollEvents();
        //toggle fullscreen if F11 pressed
        if (glfwGetKey(renderer.getVulkanEngine().getWindow(), GLFW_KEY_F11) == GLFW_PRESS && (!lastF11)) {
            if (windowFullScreen) {
                glfwSetWindowMonitor(renderer.getVulkanEngine().getWindow(), nullptr,
                    smallScreenWindowPos[0], smallScreenWindowPos[1],
                    smallScreenWindowDimensions[0], smallScreenWindowDimensions[1],
                    displayMode->refreshRate );
            }
            else {
                smallScreenWindowDimensions[0] = windowDimensions[0];
                smallScreenWindowDimensions[1] = windowDimensions[1];
                glfwGetWindowPos(renderer.getVulkanEngine().getWindow(), &smallScreenWindowPos[0], &smallScreenWindowPos[1]);
                glfwSetWindowMonitor(renderer.getVulkanEngine().getWindow(), glfwGetPrimaryMonitor(),
                    0, 0, displayMode->width, displayMode->height, displayMode->refreshRate );
            }

            windowFullScreen = !windowFullScreen;
        }
        lastF11 = glfwGetKey(renderer.getVulkanEngine().getWindow(), GLFW_KEY_F11) == GLFW_PRESS;
        int windowSize[2];
        glfwGetWindowSize(renderer.getVulkanEngine().getWindow(), &windowSize[0], &windowSize[1]);
        if (windowDimensions[0] != windowSize[0] || windowDimensions[1] != windowSize[1]) {
            windowDimensions[0] = windowSize[0];
            windowDimensions[1] = windowSize[1];
        }
        lastLastWindowFullScreen = lastWindowFullScreen;
        lastWindowFullScreen = windowFullScreen;

        // Render if a frame is needed
        end = std::chrono::steady_clock::now();
        double currentTime = (double)std::chrono::duration_cast<std::chrono::microseconds>
            (end - start).count() / 1000000;
        if (currentTime > frameStart + DT) {
            double actualDT = currentTime - frameStart;
            if (currentTime - lastFrameRateTime > 1) {
                LOG(
                    std::to_string(
                        renderer.getVulkanEngine().getCurrentFrame() - lastFrameRateFrames
                    ) + " FPS"
                );
                LOG(
                    std::to_string(
                        mainPlayer.viewCamera.position[0] + mainPlayer.cameraBlockPosition[0]
                    ) + ", " + std::to_string(
                        mainPlayer.viewCamera.position[1] + mainPlayer.cameraBlockPosition[1]
                    ) + ", " + std::to_string(
                        mainPlayer.viewCamera.position[2] + mainPlayer.cameraBlockPosition[2]
                    )
                );
                lastFrameRateTime += 1;
                lastFrameRateFrames = renderer.getVulkanEngine().getCurrentFrame();
            }
            //update frame rate limiter
            if ((currentTime - DT) < (frameStart + DT)) {
                frameStart += DT;
            }
            else {
                frameStart = currentTime;
            }

            mainPlayer.processUserInput(
                renderer.getVulkanEngine().getWindow(), windowDimensions, &windowLastFocus,
                currentTime, networking
            );
            mainWorld.updateMeshes();
            mainWorld.updatePlayerPos(
                mainPlayer.cameraBlockPosition, &(mainPlayer.viewCamera.position[0])
            );

            //create model view projection matrix for the world
            float fov = 60.0f;
            fov = fov - fov * (2.0f / 3) * mainPlayer.zoom;
            glm::mat4 projection = glm::perspective(
                glm::radians(fov), ((float)windowDimensions[0] / windowDimensions[1]), 0.1f,
                (float)((mainWorld.getRenderDistance() - 1) * constants::CHUNK_SIZE)
            );
            glm::mat4 projectionReversedDepth = glm::perspective(
                glm::radians(fov), ((float)windowDimensions[0] / windowDimensions[1]),
                (float)((mainWorld.getRenderDistance() - 1) * constants::CHUNK_SIZE), 0.1f
            );
            glm::mat4 view;
            mainPlayer.viewCamera.getViewMatrix(&view);
            glm::mat4 viewProjection = projectionReversedDepth * view;

            uint32_t timeOfDay =
                (mainWorld.integratedServer.getTickNum() + constants::DAY_LENGTH / 4) %
                constants::DAY_LENGTH;
            float groundLuminance = calculateBrightness(
                constants::GROUND_LUMINANCE, constants::NUM_GROUND_LUMINANCE_POINTS, timeOfDay
            );
            // LOG(std::to_string(timeOfDay) + ": " + std::to_string(groundLuminance));

            glm::vec3 sunDirection(glm::cos((float)((timeOfDay + constants::DAY_LENGTH * 3 / 4) % constants::DAY_LENGTH) /
                constants::DAY_LENGTH * glm::pi<float>() * 2), glm::sin((float)((timeOfDay + constants::DAY_LENGTH * 3 / 4) % constants::DAY_LENGTH) /
                constants::DAY_LENGTH * glm::pi<float>() * 2), 0.0f);
            renderer.skyRenderInfo.sunDir = sunDirection;
            renderer.skyRenderInfo.inverseViewProjection = glm::inverse(
                projection * glm::lookAt(
                    glm::vec3(0.0f), mainPlayer.viewCamera.front, -mainPlayer.viewCamera.up
            ));
            renderer.skyRenderInfo.brightness = groundLuminance;
            renderer.skyRenderInfo.sunGlowColour = glm::vec3(1.7f, 0.67f, 0.13f);
            renderer.skyRenderInfo.sunGlowAmount = std::pow(
                std::abs(glm::dot(sunDirection, glm::vec3(1.0f, 0.0f, 0.0f))), 64.0f
            );

            renderer.beginRenderingFrame();
            if (!renderer.isMinimised())
            {
                renderer.drawSky();
                mainWorld.buildEntityMesh(mainPlayer.cameraBlockPosition);
                renderer.beginDrawingGeometry();
                renderer.blitSky();

                // Render the world geometry
                float cameraSubBlockPos[3];
                mainPlayer.viewCamera.getPosition(cameraSubBlockPos);
                FrameData& currentFrameData = renderer.getVulkanEngine().getCurrentFrameData();
                VkCommandBuffer command = currentFrameData.commandBuffer;
                #ifdef TIMESTAMPS
                vkCmdWriteTimestamp(
                    command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, renderer.getVulkanEngine().getCurrentTimestampQueryPool(), 0
                );
                #endif
                mainWorld.renderWorld(
                    viewProjection, mainPlayer.cameraBlockPosition,
                    glm::vec3(cameraSubBlockPos[0], cameraSubBlockPos[1], cameraSubBlockPos[2]),
                    (float)windowDimensions[0] / (float)windowDimensions[1], fov, groundLuminance,
                    actualDT
                );
                #ifdef TIMESTAMPS
                vkCmdWriteTimestamp(
                    command, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, renderer.getVulkanEngine().getCurrentTimestampQueryPool(), 1
                );
                #endif

                // //auto tp2 = std::chrono::high_resolution_clock::now();
                // //LOG(std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count()) + "us");

                // Draw the block outline
                int breakBlockCoords[3];
                int placeBlockCoords[3];
                uint8_t lookingAtBlock = mainWorld.shootRay(mainPlayer.viewCamera.position,
                    mainPlayer.cameraBlockPosition, mainPlayer.viewCamera.front,
                    breakBlockCoords, placeBlockCoords); if (lookingAtBlock) {
                    //create the model view projection matrix for the outline
                    glm::vec3 offset;
                    for (uint8_t i = 0; i < 3; i++)
                        offset[i] = breakBlockCoords[i] - mainPlayer.cameraBlockPosition[i];
                    offset += glm::vec3(0.5f, 0.5f, 0.5f);
                    renderer.drawBlockOutline(
                        viewProjection, offset,
                        mainWorld.integratedServer.getResourcePack().getBlockData(
                            lookingAtBlock).model->boundingBoxVertices
                    );
                }

                renderer.finishDrawingGeometry();
                renderer.renderBloom();
                renderer.calculateAutoExposure(actualDT);
                renderer.applyToneMap();
                renderer.drawCrosshair();

                // font.queue(testText, glm::ivec2(100, 100), 3, glm::vec3(1.0f, 1.0f, 1.0f));
                // font.draw(mainRenderer);

                renderer.submitFrame();
            }
        }
        mainWorld.doRenderThreadJobs();

        if (glfwWindowShouldClose(renderer.getVulkanEngine().getWindow()))
        {
            do
            {
                running = false;
                for (int8_t i = 0; i < mainWorld.getNumChunkLoaderThreads(); i++) {
                    running |= chunkLoaderThreadsRunning[i];
                }
                mainWorld.doRenderThreadJobs();
            } while (running);
        }
    }

    vkDeviceWaitIdle(renderer.getVulkanEngine().getDevice());
    mainWorld.unloadAllMeshes();
    mainWorld.freeEntityMeshes();

    logicWorker.join();

    delete[] chunkLoaderThreadsRunning;

    if (multiplayer) {
        networking.disconnect(mainWorld);
        enet_deinitialize();
    }
}

}  // namespace lonelycube::client
