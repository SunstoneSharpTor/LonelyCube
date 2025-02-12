/*
  Lonely Cube, a voxel game
  Copyright (C) g 2024-2025 Bertie Cartwright

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "client/renderThread.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <enet/enet.h>
#include "stb_image.h"

#include "core/pch.h"

#include <limits>
#include <string>
#include <time.h>

#include "client/logicThread.h"
#include "client/clientNetworking.h"
#include "client/graphics/bloom.h"
#include "client/graphics/computeShader.h"
#include "client/graphics/frameBuffer.h"
#include "client/graphics/luminance.h"
#include "client/graphics/glRenderer.h"
#include "client/graphics/renderer.h"
#include "client/graphics/vertexBuffer.h"
#include "client/graphics/indexBuffer.h"
#include "client/graphics/vertexArray.h"
#include "client/graphics/vulkan/vulkanEngine.h"
#include "client/graphics/shader.h"
#include "client/graphics/texture.h"
#include "client/graphics/camera.h"
#include "client/gui/font.h"
#include "client/clientWorld.h"
#include "client/clientPlayer.h"
#include "core/config.h"
#include "core/constants.h"
#include "core/log.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

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

    uint32_t worldSeed = std::time(0);
    int playerSpawnPoint[3] = { 0, 200, 0 };
    ClientWorld mainWorld(
        settings.getRenderDistance(), worldSeed, !multiplayer, playerSpawnPoint,
        multiplayer ? networking.getPeer() : nullptr, networking.getMutex()
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

    {
        {
        Renderer renderer;

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
            renderer.getVulkanEngine().getWindow(), windowDimensions, &windowLastFocus, &running,
            time, networking
        );
        mainWorld.doRenderThreadJobs();

        //set up game loop
        float exposure = 0.0;
        float exposureTimeByDTs = 0.0;
        int FPS_CAP = std::numeric_limits<int>::max();
        double DT = 1.0 / FPS_CAP;
        uint64_t frames = 0;
        uint64_t lastFrameRateFrames = 0;
        auto start = std::chrono::steady_clock::now();
        auto end = start;
        time = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000;
        double frameStart = time - DT;
        float lastFrameRateTime = frameStart + DT;
        bool run = running;
        while (run) {
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
            if (glfwWindowShouldClose(renderer.getVulkanEngine().getWindow()))
                running = false;
            int windowSize[2];
            glfwGetWindowSize(renderer.getVulkanEngine().getWindow(), &windowSize[0], &windowSize[1]);
            if (windowDimensions[0] != windowSize[0] || windowDimensions[1] != windowSize[1]) {
                windowDimensions[0] = windowSize[0];
                windowDimensions[1] = windowSize[1];
                // worldFrameBuffer.resize(reinterpret_cast<uint32_t*>(windowDimensions));
                // glViewport(0, 0, windowDimensions[0], windowDimensions[1]);
                // font.resize(reinterpret_cast<uint32_t*>(windowDimensions));
                // #ifndef GLES3
                // glBindTexture(GL_TEXTURE_2D, skyTexture);
                // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowDimensions[0], windowDimensions[1], 0, GL_RGBA, GL_FLOAT, NULL);
                // bloom.resize(reinterpret_cast<uint32_t*>(windowDimensions));
                // luminance.resize(reinterpret_cast<uint32_t*>(windowDimensions));
                // #else
                // skyFrameBuffer.resize(windowDimensions);
                // #endif
                // crosshairProj = glm::ortho(-(float)windowDimensions[0] / 2, (float)windowDimensions[0] / 2, -(float)windowDimensions[1] / 2, (float)windowDimensions[1] / 2, -1.0f, 1.0f);
                // crosshairShader.bind();
                // crosshairShader.setUniformMat4f("u_MVP", crosshairProj);
            }
            lastLastWindowFullScreen = lastWindowFullScreen;
            lastWindowFullScreen = windowFullScreen;

            //render if a frame is needed
            // GLPrintErrors();
            end = std::chrono::steady_clock::now();
            double currentTime = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000;
            if (currentTime > frameStart + DT) {
                double actualDT = currentTime - frameStart;
                if (currentTime - lastFrameRateTime > 1) {
                    LOG(std::to_string(frames - lastFrameRateFrames) + " FPS");
                    LOG(std::to_string(mainPlayer.viewCamera.position[0] + mainPlayer.cameraBlockPosition[0]) + ", "
                        + std::to_string(mainPlayer.viewCamera.position[1] + mainPlayer.cameraBlockPosition[1]) + ", "
                        + std::to_string(mainPlayer.viewCamera.position[2] + mainPlayer.cameraBlockPosition[2]));
                    lastFrameRateTime += 1;
                    lastFrameRateFrames = frames;
                }
                //update frame rate limiter
                if ((currentTime - DT) < (frameStart + DT)) {
                    frameStart += DT;
                }
                else {
                    frameStart = currentTime;
                }

                mainPlayer.processUserInput(renderer.getVulkanEngine().getWindow(), windowDimensions, &windowLastFocus, &running, currentTime, networking);
                mainWorld.updatePlayerPos(mainPlayer.cameraBlockPosition, &(mainPlayer.viewCamera.position[0]));

                //create model view projection matrix for the world
                float fov = 70.0;
                fov = fov - fov * (2.0 / 3.0) * mainPlayer.zoom;
                glm::mat4 projection = glm::perspective(glm::radians(fov), ((float)windowDimensions[0] / (float)windowDimensions[1]), 0.12f, (float)((mainWorld.getRenderDistance() - 1) * constants::CHUNK_SIZE));
                glm::mat4 inverseProjection = glm::inverse(projection);
                glm::mat4 view;
                mainPlayer.viewCamera.getViewMatrix(&view);
                glm::mat4 inverseView = glm::inverse(view);
                glm::mat4 model = (glm::mat4(1.0f));
                glm::mat4 mvp = projection * view * model;
                // //update the MVP uniform
                // blockShader.bind();
                // blockShader.setUniformMat4f("u_modelView", view * model);
                // blockShader.setUniformMat4f("u_proj", projection);
                //
                // // Set up block outline
                // int breakBlockCoords[3];
                // int placeBlockCoords[3];
                // uint8_t lookingAtBlock = mainWorld.shootRay(mainPlayer.viewCamera.position,
                //     mainPlayer.cameraBlockPosition, mainPlayer.viewCamera.front,
                //     breakBlockCoords, placeBlockCoords);
                // if (lookingAtBlock) {
                //     //create the model view projection matrix for the outline
                //     glm::vec3 outlinePosition;
                //     for (uint8_t i = 0; i < 3; i++) {
                //         outlinePosition[i] = breakBlockCoords[i] - mainPlayer.cameraBlockPosition[i];
                //     }
                //     model = glm::translate(model, outlinePosition);
                //     glm::mat4 mvp = projection * view * model;
                //     blockOutlineShader.bind();
                //     blockOutlineShader.setUniformMat4f("u_MVP", mvp);
                // }

                uint32_t timeOfDay = (mainWorld.integratedServer.getTickNum() + constants::DAY_LENGTH / 4) % constants::DAY_LENGTH;
                // Calculate ground luminance
                float groundLuminance = calculateBrightness(constants::GROUND_LUMINANCE, constants::NUM_GROUND_LUMINANCE_POINTS, timeOfDay);
                // LOG(std::to_string(timeOfDay) + ": " + std::to_string(groundLuminance));

                glm::vec3 sunDirection(glm::cos((float)((timeOfDay + constants::DAY_LENGTH * 3 / 4) % constants::DAY_LENGTH) /
                    constants::DAY_LENGTH * glm::pi<float>() * 2), glm::sin((float)((timeOfDay + constants::DAY_LENGTH * 3 / 4) % constants::DAY_LENGTH) /
                    constants::DAY_LENGTH * glm::pi<float>() * 2), 0.0f);
                renderer.skyRenderInfo.sunDir = sunDirection;
                renderer.skyRenderInfo.inverseViewProjection = inverseView * inverseProjection;
                renderer.skyRenderInfo.brightness = groundLuminance;
                renderer.skyRenderInfo.sunGlowColour = glm::vec3(1.5f, 0.6f, 0.13f);
                renderer.skyRenderInfo.sunGlowAmount = std::pow(
                    std::abs(glm::dot(sunDirection, glm::vec3(1.0f, 0.0f, 0.0f))), 32.0f
                );
                renderer.drawFrame();

                // // Render the world to a texture
                // worldFrameBuffer.bind();
                // mainRenderer.clear();
                // #ifndef GLES3
                // // Draw the sky
                // glBindImageTexture(1, worldFrameBuffer.getTextureColourBuffer(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
                // skyBlitShader.bind();
                // glDispatchCompute((uint32_t)((windowDimensions[0] + 7) / 8),
                //   (uint32_t)((windowDimensions[1] + 7) / 8), 1);
                // // Make sure writing to image has finished before read
                // glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                //
                // // Draw the sun
                // glBindImageTexture(0, worldFrameBuffer.getTextureColourBuffer(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
                // sunShader.bind();
                // sunShader.setUniformVec3("sunDir", sunDirection);
                // sunShader.setUniformMat4f("inverseProjection", inverseProjection);
                // sunShader.setUniformMat4f("inverseView", inverseView);
                // sunShader.setUniform1f("brightness", groundLuminance * 1000);
                // glDispatchCompute((uint32_t)((windowDimensions[0] + 7) / 8),
                //     (uint32_t)((windowDimensions[1] + 7) / 8), 1);
                // // Make sure writing to image has finished before read
                // glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                // #endif
                //
                // // Render the world geometry
                // glEnable(GL_DEPTH_TEST);
                // worldTextures.bind();
                // glActiveTexture(GL_TEXTURE1);
                // #ifndef GLES3
                // glBindTexture(GL_TEXTURE_2D, skyTexture);
                // #else
                // glBindTexture(GL_TEXTURE_2D, skyFrameBuffer.getTextureColourBuffer());
                // #endif
                // //auto tp1 = std::chrono::high_resolution_clock::now();
                // mainWorld.renderWorld(mainRenderer, blockShader, waterShader, view, projection,
                //     mainPlayer.cameraBlockPosition, (float)windowDimensions[0] /
                //     (float)windowDimensions[1], fov, groundLuminance, actualDT);
                // //auto tp2 = std::chrono::high_resolution_clock::now();
                // //LOG(std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count()) + "us");
                //
                // //auto tp1 = std::chrono::high_resolution_clock::now();
                // #ifndef GLES3
                // bloom.render(0.005f, 0.005f);
                // #endif
                // //auto tp2 = std::chrono::high_resolution_clock::now();
                // //LOG(std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count()) + "us");
                //
                // // Draw the block outline
                // if (lookingAtBlock) {
                //     VertexArray blockOutlineVA;
                //     VertexBuffer blockOutlineVB((mainWorld).integratedServer.getResourcePack().getBlockData(
                //         lookingAtBlock).model->boundingBoxVertices, 24 * sizeof(float));
                //     blockOutlineVA.addBuffer(blockOutlineVB, blockOutlineVBL);
                //     mainRenderer.drawWireframe(blockOutlineVA, blockOutlineIB, blockOutlineShader);
                // }
                // worldFrameBuffer.unbind();

                // // Update auto exposure
                // #ifndef GLES3
                // float luminanceVal = luminance.calculate();
                // #else
                const float minDarknessAmbientLight = 0.00002f;
                float maxDarknessAmbientLight = std::min(0.001f, groundLuminance);
                float skyLightLevel = (float)mainWorld.integratedServer.chunkManager.getSkyLight(
                    mainPlayer.cameraBlockPosition) / constants::skyLightMaxValue;
                float factor = skyLightLevel * skyLightLevel * skyLightLevel;
                float skyLightBrightness = groundLuminance / (1.0f + (1.0f - skyLightLevel) * (1.0f - skyLightLevel) * 45.0f)
                    * factor + maxDarknessAmbientLight / (1.0f + (1.0f - skyLightLevel) * (1.0f - skyLightLevel) *
                    45.0f) * (1.0f - factor);
                float luminanceVal = std::max(skyLightBrightness, minDarknessAmbientLight) * 0.1f;
                // #endif
                float targetExposure = std::max(1.0f / 10.0f, std::min(0.2f / luminanceVal, 1.0f / 0.005f));
                exposureTimeByDTs += actualDT;
                while (exposureTimeByDTs > (1.0/(double)constants::visualTPS)) {
                    float fac = 0.008;
                    exposure += ((targetExposure > exposure) * 2 - 1) * std::min(
                        std::abs(targetExposure - exposure),
                        (targetExposure - exposure) * (targetExposure - exposure) * fac
                    );
                    exposureTimeByDTs -= (1.0/(float)constants::visualTPS);
                }
                // screenShader.bind();
                // screenShader.setUniform1f("exposure", exposure);

                // // Draw the world texture
                // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                // glClear(GL_COLOR_BUFFER_BIT);
                // glDisable(GL_DEPTH_TEST);
                // worldFrameBuffer.draw(screenShader);
                //
                // // Draw the crosshair
                // glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
                // glActiveTexture(GL_TEXTURE0);
                // mainRenderer.draw(crosshairVA, crosshairIB, crosshairShader);
                // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                //
                // font.queue(testText, glm::ivec2(100, 100), 3, glm::vec3(1.0f, 1.0f, 1.0f));
                // font.draw(mainRenderer);
                //
                // glfwSwapBuffers(window);
                //
                // frames++;
            }
            mainWorld.updateMeshes();
            mainWorld.doRenderThreadJobs();

            run = running;
            for (int8_t i = 0; i < mainWorld.getNumChunkLoaderThreads(); i++) {
                run |= chunkLoaderThreadsRunning[i];
            }
        }
    }

    logicWorker.join();

    delete[] chunkLoaderThreadsRunning;

    if (multiplayer) {
        std::lock_guard<std::mutex> lock(networking.getMutex());
        enet_peer_disconnect(networking.getPeer(), 0);
        ENetEvent event;
        while (enet_host_service(networking.getHost(), &event, 3000) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
                case ENET_EVENT_TYPE_DISCONNECT:
                LOG("Disconnection succeeded!");
                break;
            }
        }

        enet_deinitialize();
    }

        const int defaultWindowDimensions[2] = { 853, 480 };
        int windowDimensions[2] = { defaultWindowDimensions[0], defaultWindowDimensions[1] };
        uint32_t smallScreenWindowDimensions[2];
        int smallScreenWindowPos[2];

        glfwInit();
        const GLFWvidmode* displayMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        windowDimensions[0] = std::min((int)windowDimensions[0], displayMode->width / 2);
        windowDimensions[1] = std::min((int)windowDimensions[1], displayMode->height/ 2);

        #ifdef GLES3
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
        #else
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        #endif
        glfwWindowHint(GLFW_DEPTH_BITS, 24);

        window = glfwCreateWindow(
            windowDimensions[0], windowDimensions[1], "Lonely Cube", NULL, NULL
        );
        glfwSetWindowPos(window, displayMode->width / 2 - windowDimensions[0] / 2,
            displayMode->height / 2 - windowDimensions[1] / 2);
        glfwMakeContextCurrent(window);

        GLFWimage images[1];
        images[0].pixels = stbi_load(
            "res/resourcePack/logo.png", &images[0].width, &images[0].height, 0, 4
        );
        glfwSetWindowIcon(window, 1, images);

        bool VSYNC = false;
        if (VSYNC)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);

        bool windowLastFocus = false;
        bool windowFullScreen = false;
        bool lastWindowFullScreen = false;
        bool lastLastWindowFullScreen = false;
        int windowRestoredSize[2];
        int windowRestoredPos[2];
        bool lastF11 = false;

        #ifdef GLES3
            gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress);
        #else
            gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        #endif

        // Create shaders
        Shader waterShader("res/shaders/blockVertex.txt", "res/shaders/waterFragment.txt");
        waterShader.bind();
        waterShader.setUniform1i("u_blockTextures", 0);
        waterShader.setUniform1i("u_skyTexture", 1);
        waterShader.setUniform1f("u_renderDistance", (mainWorld.getRenderDistance() - 1) * constants::CHUNK_SIZE);
        Shader blockShader("res/shaders/blockVertex.txt", "res/shaders/blockFragment.txt");
        blockShader.bind();
        blockShader.setUniform1i("u_blockTextures", 0);
        blockShader.setUniform1i("u_skyTexture", 1);
        blockShader.setUniform1f("u_renderDistance", (mainWorld.getRenderDistance() - 1) * constants::CHUNK_SIZE);
        Shader blockOutlineShader("res/shaders/wireframeVertex.txt", "res/shaders/wireframeFragment.txt");
        Shader crosshairShader("res/shaders/crosshairVertex.txt", "res/shaders/crosshairFragment.txt");
        crosshairShader.bind();
        glm::mat4 crosshairProj = glm::ortho(-(float)windowDimensions[0] / 2, (float)windowDimensions[0] / 2, -(float)windowDimensions[1] / 2, (float)windowDimensions[1] / 2, -1.0f, 1.0f);
        crosshairShader.setUniformMat4f("u_MVP", crosshairProj);
        Shader screenShader("res/shaders/screenShaderVertex.txt", "res/shaders/screenShaderFragment.txt");
        screenShader.bind();
        screenShader.setUniform1i("screenTexture", 0);
        screenShader.setUniform1f("exposure", 1.0f);
        #ifndef GLES3
        ComputeShader skyShader("res/shaders/sky.txt");
        ComputeShader skyBlitShader("res/shaders/skyBlit.txt");
        ComputeShader sunShader("res/shaders/sun.txt");
        ComputeShader bloomDownsampleShader("res/shaders/bloomDownsample.txt");
        bloomDownsampleShader.bind();
        bloomDownsampleShader.setUniform1i("srcTexture", 0);
        ComputeShader bloomUpsampleShader("res/shaders/bloomUpsample.txt");
        bloomUpsampleShader.bind();
        bloomUpsampleShader.setUniform1i("srcTexture", 0);
        ComputeShader bloomBlitShader("res/shaders/bloomBlit.txt");
        bloomBlitShader.bind();
        bloomBlitShader.setUniform1i("srcTexture", 0);
        ComputeShader logLuminanceDownsampleShader("res/shaders/logLuminanceDownsample.txt");
        ComputeShader simpleDownsampleShader("res/shaders/simpleDownsample.txt");
        simpleDownsampleShader.bind();
        simpleDownsampleShader.setUniform1i("srcTexture", 0);
        #endif

        Texture worldTextures("res/resourcePack/textures.png");

        GlRenderer mainRenderer;

        mainRenderer.setOpenGlOptions();

        //set up crosshair
        float crosshairCoordinates[24] = { -1.0f,  8.0f,
                                            1.0f,  8.0f,
                                            1.0f, -8.0f,
                                           -1.0f, -8.0f,

                                           -8.0f,  1.0f,
                                           -1.0f,  1.0f,
                                           -1.0f, -1.0f,
                                           -8.0f, -1.0f,

                                            8.0f,  1.0f,
                                            1.0f,  1.0f,
                                            1.0f, -1.0f,
                                            8.0f, -1.0f };

        uint32_t crosshairIndices[18] = { 2,  1,  0,
                                              0,  3,  2,
                                              6,  5,  4,
                                              4,  7,  6,
                                             10,  8,  9,
                                              8, 10, 11 };

        VertexArray crosshairVA;
        VertexBuffer crosshairVB(crosshairCoordinates, 24 * sizeof(float));
        VertexBufferLayout crosshairVBlayout;
        crosshairVBlayout.push<float>(2);
        crosshairVA.addBuffer(crosshairVB, crosshairVBlayout);
        IndexBuffer crosshairIB(crosshairIndices, 18);

        //set up block outline
        VertexBufferLayout blockOutlineVBL;
        blockOutlineVBL.push<float>(3);
        IndexBuffer blockOutlineIB(constants::CUBE_WIREFRAME_IB, 16);

        FrameBuffer<true> worldFrameBuffer(reinterpret_cast<uint32_t*>(windowDimensions));
        worldFrameBuffer.unbind();
        #ifndef GLES3
        Bloom bloom(
            worldFrameBuffer.getTextureColourBuffer(),
            reinterpret_cast<uint32_t*>(windowDimensions), bloomDownsampleShader,
            bloomUpsampleShader, bloomBlitShader
        );
        Luminance luminance(
            worldFrameBuffer.getTextureColourBuffer(),
            reinterpret_cast<uint32_t*>(windowDimensions), logLuminanceDownsampleShader,
            simpleDownsampleShader
        );

        uint32_t skyTexture;
        glGenTextures(1, &skyTexture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skyTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowDimensions[0], windowDimensions[1], 0, GL_RGBA, GL_FLOAT, NULL);
        #else
        FrameBuffer<false> skyFrameBuffer(windowDimensions);
        #endif

        mainWorld.updatePlayerPos(mainPlayer.cameraBlockPosition, &(mainPlayer.viewCamera.position[0]));

        double time;
        mainPlayer.processUserInput(window, windowDimensions, &windowLastFocus, &running, time, networking);
        mainWorld.doRenderThreadJobs();

        mainWorld.initialiseEntityRenderBuffers();

        Font font("res/resourcePack/gui/font.png", reinterpret_cast<uint32_t*>(windowDimensions));
        // glfwSetCharCallback(window, characterCallback);

        //set up game loop
        float exposure = 0.0;
        float exposureTimeByDTs = 0.0;
        int FPS_CAP = VSYNC ? displayMode->refreshRate : 10000;
        double DT = 1.0 / FPS_CAP;
        uint64_t frames = 0;
        uint64_t lastFrameRateFrames = 0;
        auto start = std::chrono::steady_clock::now();
        auto end = start;
        time = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000;
        double frameStart = time - DT;
        float lastFrameRateTime = frameStart + DT;
        bool loopRunning = running;
        while (loopRunning) {
            glfwPollEvents();
            //toggle fullscreen if F11 pressed
            if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS && (!lastF11)) {
                if (windowFullScreen) {
                    glfwSetWindowMonitor(window, nullptr,
                        smallScreenWindowPos[0], smallScreenWindowPos[1],
                        smallScreenWindowDimensions[0], smallScreenWindowDimensions[1],
                        displayMode->refreshRate );
                }
                else {
                    smallScreenWindowDimensions[0] = windowDimensions[0];
                    smallScreenWindowDimensions[1] = windowDimensions[1];
                    glfwGetWindowPos(window, &smallScreenWindowPos[0], &smallScreenWindowPos[1]);
                    glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(),
                        0, 0, displayMode->width, displayMode->height, displayMode->refreshRate );
                }

                windowFullScreen = !windowFullScreen;
            }
            lastF11 = glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS;
            if (glfwWindowShouldClose(window))
                running = false;
            int windowSize[2];
            glfwGetWindowSize(window, &windowSize[0], &windowSize[1]);
            if (windowDimensions[0] != windowSize[0] || windowDimensions[1] != windowSize[1]) {
                windowDimensions[0] = windowSize[0];
                windowDimensions[1] = windowSize[1];
                worldFrameBuffer.resize(reinterpret_cast<uint32_t*>(windowDimensions));
                glViewport(0, 0, windowDimensions[0], windowDimensions[1]);
                font.resize(reinterpret_cast<uint32_t*>(windowDimensions));
                #ifndef GLES3
                glBindTexture(GL_TEXTURE_2D, skyTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, windowDimensions[0], windowDimensions[1], 0, GL_RGBA, GL_FLOAT, NULL);
                bloom.resize(reinterpret_cast<uint32_t*>(windowDimensions));
                luminance.resize(reinterpret_cast<uint32_t*>(windowDimensions));
                #else
                skyFrameBuffer.resize(windowDimensions);
                #endif
                crosshairProj = glm::ortho(-(float)windowDimensions[0] / 2, (float)windowDimensions[0] / 2, -(float)windowDimensions[1] / 2, (float)windowDimensions[1] / 2, -1.0f, 1.0f);
                crosshairShader.bind();
                crosshairShader.setUniformMat4f("u_MVP", crosshairProj);
            }
            lastLastWindowFullScreen = lastWindowFullScreen;
            lastWindowFullScreen = windowFullScreen;

            //render if a frame is needed
            GLPrintErrors();
            end = std::chrono::steady_clock::now();
            double currentTime = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000;
            if (currentTime > frameStart + DT) {
                double actualDT = currentTime - frameStart;
                if (currentTime - lastFrameRateTime > 1) {
                    LOG(std::to_string(frames - lastFrameRateFrames) + " FPS");
                    LOG(std::to_string(mainPlayer.viewCamera.position[0] + mainPlayer.cameraBlockPosition[0]) + ", "
                        + std::to_string(mainPlayer.viewCamera.position[1] + mainPlayer.cameraBlockPosition[1]) + ", "
                        + std::to_string(mainPlayer.viewCamera.position[2] + mainPlayer.cameraBlockPosition[2]));
                    lastFrameRateTime += 1;
                    lastFrameRateFrames = frames;
                }
                //update frame rate limiter
                if ((currentTime - DT) < (frameStart + DT)) {
                    frameStart += DT;
                }
                else {
                    frameStart = currentTime;
                }

                mainPlayer.processUserInput(window, windowDimensions, &windowLastFocus, &running, currentTime, networking);
                mainWorld.updatePlayerPos(mainPlayer.cameraBlockPosition, &(mainPlayer.viewCamera.position[0]));

                //create model view projection matrix for the world
                float fov = 70.0;
                fov = fov - fov * (2.0 / 3.0) * mainPlayer.zoom;
                glm::mat4 projection = glm::perspective(glm::radians(fov), ((float)windowDimensions[0] / (float)windowDimensions[1]), 0.12f, (float)((mainWorld.getRenderDistance() - 1) * constants::CHUNK_SIZE));
                glm::mat4 inverseProjection = glm::inverse(projection);
                glm::mat4 view;
                mainPlayer.viewCamera.getViewMatrix(&view);
                glm::mat4 inverseView = glm::inverse(view);
                glm::mat4 model = (glm::mat4(1.0f));
                glm::mat4 mvp = projection * view * model;
                //update the MVP uniform
                blockShader.bind();
                blockShader.setUniformMat4f("u_modelView", view * model);
                blockShader.setUniformMat4f("u_proj", projection);

                // Set up block outline
                int breakBlockCoords[3];
                int placeBlockCoords[3];
                uint8_t lookingAtBlock = mainWorld.shootRay(mainPlayer.viewCamera.position,
                    mainPlayer.cameraBlockPosition, mainPlayer.viewCamera.front,
                    breakBlockCoords, placeBlockCoords);
                if (lookingAtBlock) {
                    //create the model view projection matrix for the outline
                    glm::vec3 outlinePosition;
                    for (uint8_t i = 0; i < 3; i++) {
                        outlinePosition[i] = breakBlockCoords[i] - mainPlayer.cameraBlockPosition[i];
                    }
                    model = glm::translate(model, outlinePosition);
                    glm::mat4 mvp = projection * view * model;
                    blockOutlineShader.bind();
                    blockOutlineShader.setUniformMat4f("u_MVP", mvp);
                }

                uint32_t timeOfDay = (mainWorld.integratedServer.getTickNum() + constants::DAY_LENGTH / 4) % constants::DAY_LENGTH;
                // Calculate ground luminance
                float groundLuminance = calculateBrightness(constants::GROUND_LUMINANCE, constants::NUM_GROUND_LUMINANCE_POINTS, timeOfDay);
                // LOG(std::to_string(timeOfDay) + ": " + std::to_string(groundLuminance));

                // Render sky
                #ifndef GLES3
                glBindImageTexture(0, skyTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
                skyShader.bind();
                glm::vec3 sunDirection(glm::cos((float)((timeOfDay + constants::DAY_LENGTH * 3 / 4) % constants::DAY_LENGTH) /
                    constants::DAY_LENGTH * glm::pi<float>() * 2), glm::sin((float)((timeOfDay + constants::DAY_LENGTH * 3 / 4) % constants::DAY_LENGTH) /
                    constants::DAY_LENGTH * glm::pi<float>() * 2), 0.0f);
                skyShader.setUniformVec3("sunDir", sunDirection);
                skyShader.setUniformMat4f("inverseProjection", inverseProjection);
                skyShader.setUniformMat4f("inverseView", inverseView);
                skyShader.setUniform1f("brightness", groundLuminance);
                skyShader.setUniformVec3("sunGlowColour", glm::vec3(1.5f, 0.6f, 0.13f));
                skyShader.setUniform1f("sunGlowAmount", std::pow(std::abs(glm::dot(sunDirection, glm::vec3(1.0f, 0.0f, 0.0f))), 32.0f));
                glDispatchCompute((uint32_t)((windowDimensions[0] + 7) / 8),
                    (uint32_t)((windowDimensions[1] + 7) / 8), 1);
                // Make sure writing to image has finished before read
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                #else
                glm::vec3 skyColour(0.3f, 0.4f, 0.95f);
                skyColour *= groundLuminance;
                skyFrameBuffer.bind();
                glClearColor(skyColour.r, skyColour.g, skyColour.b, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                #endif

                // Render the world to a texture
                worldFrameBuffer.bind();
                mainRenderer.clear();
                #ifndef GLES3
                // Draw the sky
                glBindImageTexture(1, worldFrameBuffer.getTextureColourBuffer(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
                skyBlitShader.bind();
                glDispatchCompute((uint32_t)((windowDimensions[0] + 7) / 8),
                  (uint32_t)((windowDimensions[1] + 7) / 8), 1);
                // Make sure writing to image has finished before read
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                // Draw the sun
                glBindImageTexture(0, worldFrameBuffer.getTextureColourBuffer(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
                sunShader.bind();
                sunShader.setUniformVec3("sunDir", sunDirection);
                sunShader.setUniformMat4f("inverseProjection", inverseProjection);
                sunShader.setUniformMat4f("inverseView", inverseView);
                sunShader.setUniform1f("brightness", groundLuminance * 1000);
                glDispatchCompute((uint32_t)((windowDimensions[0] + 7) / 8),
                    (uint32_t)((windowDimensions[1] + 7) / 8), 1);
                // Make sure writing to image has finished before read
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                #endif

                // Render the world geometry
                glEnable(GL_DEPTH_TEST);
                worldTextures.bind();
                glActiveTexture(GL_TEXTURE1);
                #ifndef GLES3
                glBindTexture(GL_TEXTURE_2D, skyTexture);
                #else
                glBindTexture(GL_TEXTURE_2D, skyFrameBuffer.getTextureColourBuffer());
                #endif
                //auto tp1 = std::chrono::high_resolution_clock::now();
                mainWorld.renderWorld(mainRenderer, blockShader, waterShader, view, projection,
                    mainPlayer.cameraBlockPosition, (float)windowDimensions[0] /
                    (float)windowDimensions[1], fov, groundLuminance, actualDT);
                //auto tp2 = std::chrono::high_resolution_clock::now();
                //LOG(std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count()) + "us");

                //auto tp1 = std::chrono::high_resolution_clock::now();
                #ifndef GLES3
                bloom.render(0.005f, 0.005f);
                #endif
                //auto tp2 = std::chrono::high_resolution_clock::now();
                //LOG(std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count()) + "us");

                // Draw the block outline
                if (lookingAtBlock) {
                    VertexArray blockOutlineVA;
                    VertexBuffer blockOutlineVB((mainWorld).integratedServer.getResourcePack().getBlockData(
                        lookingAtBlock).model->boundingBoxVertices, 24 * sizeof(float));
                    blockOutlineVA.addBuffer(blockOutlineVB, blockOutlineVBL);
                    mainRenderer.drawWireframe(blockOutlineVA, blockOutlineIB, blockOutlineShader);
                }
                worldFrameBuffer.unbind();

                // Update auto exposure
                #ifndef GLES3
                float luminanceVal = luminance.calculate();
                #else
                const float minDarknessAmbientLight = 0.00002f;
                float maxDarknessAmbientLight = std::min(0.001f, groundLuminance);
                float skyLightLevel = (float)mainWorld.integratedServer.chunkManager.getSkyLight(
                    mainPlayer.cameraBlockPosition) / constants::skyLightMaxValue;
                float factor = skyLightLevel * skyLightLevel * skyLightLevel;
                float skyLightBrightness = groundLuminance / (1.0f + (1.0f - skyLightLevel) * (1.0f - skyLightLevel) * 45.0f)
                    * factor + maxDarknessAmbientLight / (1.0f + (1.0f - skyLightLevel) * (1.0f - skyLightLevel) *
                    45.0f) * (1.0f - factor);
                float luminanceVal = std::max(skyLightBrightness, minDarknessAmbientLight) * 0.1f;
                #endif
                float targetExposure = std::max(1.0f / 10.0f, std::min(0.2f / luminanceVal, 1.0f / 0.005f));
                exposureTimeByDTs += actualDT;
                while (exposureTimeByDTs > (1.0/(double)constants::visualTPS)) {
                    float fac = 0.008;
                    exposure += ((targetExposure > exposure) * 2 - 1) * std::min(
                        std::abs(targetExposure - exposure),
                        (targetExposure - exposure) * (targetExposure - exposure) * fac
                    );
                    exposureTimeByDTs -= (1.0/(float)constants::visualTPS);
                }
                screenShader.bind();
                screenShader.setUniform1f("exposure", exposure);

                // Draw the world texture
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                glDisable(GL_DEPTH_TEST);
                worldFrameBuffer.draw(screenShader);

                // Draw the crosshair
                glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
                glActiveTexture(GL_TEXTURE0);
                mainRenderer.draw(crosshairVA, crosshairIB, crosshairShader);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                font.queue(testText, glm::ivec2(100, 100), 3, glm::vec3(1.0f, 1.0f, 1.0f));
                font.draw(mainRenderer);

                glfwSwapBuffers(window);

                frames++;
            }
            mainWorld.updateMeshes();
            mainWorld.doRenderThreadJobs();

            loopRunning = running;
            for (int8_t i = 0; i < mainWorld.getNumChunkLoaderThreads(); i++) {
                loopRunning |= chunkLoaderThreadsRunning[i];
            }
        }
    }
    mainWorld.deinitialiseEntityRenderBuffers();
    glfwDestroyWindow(window);
    glfwTerminate();

    logicWorker.join();

    delete[] chunkLoaderThreadsRunning;

    if (multiplayer) {
        std::lock_guard<std::mutex> lock(networking.getMutex());
        enet_peer_disconnect(networking.getPeer(), 0);
        ENetEvent event;
        while (enet_host_service(networking.getHost(), &event, 3000) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
                case ENET_EVENT_TYPE_DISCONNECT:
                LOG("Disconnection succeeded!");
                break;
            }
        }

        enet_deinitialize();
    }
}

}  // namespace lonelycube::client
