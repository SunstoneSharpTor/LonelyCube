/*
  Lonely Cube, a voxel game
  Copyright (C) 2024 Bertie Cartwright

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
#include <SDL2/SDL.h>
#include <enet/enet.h>

#include "core/pch.h"
#include <time.h>

#include "client/clientNetworking.h"
#include "client/computeShader.h"
#include "client/frameBuffer.h"
#include "client/renderer.h"
#include "client/vertexBuffer.h"
#include "client/indexBuffer.h"
#include "client/vertexArray.h"
#include "client/shader.h"
#include "client/texture.h"
#include "client/camera.h"
#include "client/clientWorld.h"
#include "client/clientPlayer.h"
#include "core/chunk.h"
#include "core/packet.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace client {

void RenderThread::go(bool* running) {
    const int defaultWindowDimensions[2] = { 853, 480 };
    int windowDimensions[2] = { defaultWindowDimensions[0], defaultWindowDimensions[1] };

    SDL_Window* sdl_window;
    SDL_GLContext context;

    SDL_Init(SDL_INIT_VIDEO);

    #ifdef GLES3
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        // Force usage of the GLES backend
        SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
    #else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    #endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    sdl_window = SDL_CreateWindow(
        "Lonely Cube",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        windowDimensions[0],
        windowDimensions[1],
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    context = SDL_GL_CreateContext(sdl_window);
    bool VSYNC = false;
    if ((!VSYNC) || SDL_GL_SetSwapInterval(-1) != 0) {
        SDL_GL_SetSwapInterval(0);
        std::cout << "vsync not enabled\n";
    }
    else {
        std::cout << "vsync enabled\n";
    }

    bool windowMaximised = false;
    bool windowLastFocus = false;
    bool windowFullScreen = false;
    bool lastWindowFullScreen = false;
    bool lastLastWindowFullScreen = false;
    int windowRestoredSize[2];
    int windowRestoredPos[2];
    SDL_GetWindowSize(sdl_window, &windowRestoredSize[0], &windowRestoredSize[1]);
    SDL_GetWindowPosition(sdl_window, &windowRestoredPos[0], &windowRestoredPos[1]);
    const Uint8* keyboardState = SDL_GetKeyboardState(NULL);
    bool lastF11 = false;

    m_mainPlayer->setWorldMouseData(sdl_window, windowDimensions);

    #ifdef GLES3
        gladLoadGLES2Loader((GLADloadproc)SDL_GL_GetProcAddress);
    #else
        gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
    #endif

    // Water shader
    Shader waterShader("res/shaders/blockVertex.txt", "res/shaders/waterFragment.txt");
    waterShader.bind();
    waterShader.setUniform1i("u_blockTextures", 0);
    waterShader.setUniform1i("u_skyTexture", 1);
    waterShader.setUniform1f("u_renderDistance", (m_mainWorld->getRenderDistance() - 1) * constants::CHUNK_SIZE);
    // Block shader
    Shader blockShader("res/shaders/blockVertex.txt", "res/shaders/blockFragment.txt");
    blockShader.bind();
    blockShader.setUniform1i("u_blockTextures", 0);
    blockShader.setUniform1i("u_skyTexture", 1);
    blockShader.setUniform1f("u_renderDistance", (m_mainWorld->getRenderDistance() - 1) * constants::CHUNK_SIZE);
    // Block outline shader
    Shader blockOutlineShader("res/shaders/wireframeVertex.txt", "res/shaders/wireframeFragment.txt");
    // Crosshair shader
    Shader crosshairShader("res/shaders/crosshairVertex.txt", "res/shaders/crosshairFragment.txt");
    crosshairShader.bind();
    glm::mat4 crosshairProj = glm::ortho(-(float)windowDimensions[0] / 2, (float)windowDimensions[0] / 2, -(float)windowDimensions[1] / 2, (float)windowDimensions[1] / 2, -1.0f, 1.0f);
    crosshairShader.setUniformMat4f("u_MVP", crosshairProj);
    // Screen shader
    Shader screenShader("res/shaders/screenShaderVertex.txt", "res/shaders/screenShaderFragment.txt");
    screenShader.bind();
    screenShader.setUniform1i("screenTexture", 0);
    // Sky shader
    ComputeShader skyShader("res/shaders/sky.txt");

    float screenCoords[24] = { -1.0f, -1.0f, 0.0f, 0.0f,
                                        1.0f,  -1.0f, 1.0f,  0.0f,
                                        -1.0f, 1.0f, 0.0f, 1.0f,
                                       -1.0f, 1.0f, 0.0f, 1.0f,
                                       1.0f,  -1.0f, 1.0f,  0.0f,
                                        1.0f,  1.0f, 1.0f,  1.0f };
    VertexBufferLayout screenVBL;
    VertexArray screenVA;
    VertexBuffer screenVB(screenCoords, 24 * sizeof(float));
    screenVBL.push<float>(2);
    screenVBL.push<float>(2);
    screenVA.addBuffer(screenVB, screenVBL);

    Texture allBlockTextures("res/blockTextures.png");

    Renderer mainRenderer;

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

    unsigned int crosshairIndices[18] = { 2,  1,  0,
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
    VertexArray blockOutlineVA;
    VertexBuffer blockOutlineVB(constants::WIREFRAME_CUBE_FACE_POSITIONS, 24 * sizeof(float));
    VertexBufferLayout blockOutlineVBL;
    blockOutlineVBL.push<float>(3);
    blockOutlineVA.addBuffer(blockOutlineVB, blockOutlineVBL);
    IndexBuffer blockOutlineIB(constants::CUBE_WIREFRAME_IB, 16);

    FrameBuffer<true> worldFrameBuffer(windowDimensions);
    worldFrameBuffer.unbind();

    // texture size
    unsigned int skyTexture;
    glGenTextures(1, &skyTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, skyTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowDimensions[0], windowDimensions[1], 0, GL_RGBA, GL_FLOAT, NULL);

    float cameraPos[3];
    cameraPos[0] = m_mainPlayer->cameraBlockPosition[0] + m_mainPlayer->viewCamera.position[0];
    cameraPos[1] = m_mainPlayer->cameraBlockPosition[1] + m_mainPlayer->viewCamera.position[1];
    cameraPos[2] = m_mainPlayer->cameraBlockPosition[2] + m_mainPlayer->viewCamera.position[2];
    m_mainWorld->updatePlayerPos(cameraPos[0], cameraPos[1], cameraPos[2]);
    
    auto start = std::chrono::steady_clock::now();
    auto end = std::chrono::steady_clock::now();
    double time = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000;
    m_mainPlayer->processUserInput(sdl_window, windowDimensions, &windowLastFocus, running, time, m_networking);
    m_mainWorld->doRenderThreadJobs();

    //set up game loop
    SDL_DisplayMode displayMode;
    SDL_GetWindowDisplayMode(sdl_window, &displayMode);
    int FPS_CAP = VSYNC ? displayMode.refresh_rate : 10000;
    double DT = 1.0 / FPS_CAP;
    unsigned long long frames = 0;
    unsigned long long lastFrameRateFrames = 0;
    end = std::chrono::steady_clock::now();
    time = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000;
    double frameStart = time - DT;
    bool windowevent_resized = false;
    float lastFrameRateTime = frameStart + DT;
    bool loopRunning = *running;
    while (loopRunning) {
        //GLPrintErrors();
        //toggle fullscreen if F11 pressed
        if (keyboardState[SDL_SCANCODE_F11] && (!lastF11)) {
            if (windowFullScreen) {
                SDL_SetWindowFullscreen(sdl_window, 0);
                if (!windowMaximised) {
                    SDL_RestoreWindow(sdl_window);
                }
            }
            else {
                if (!windowMaximised) {
                    SDL_MaximizeWindow(sdl_window);
                }
                SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_FULLSCREEN);
            }

            windowFullScreen = !windowFullScreen;
        }
        lastF11 = keyboardState[SDL_SCANCODE_F11];
        //poll events
        SDL_Event windowEvent;
        Uint32 windowFlags;
        while (SDL_PollEvent(&windowEvent)) {
            switch (windowEvent.type) {
            case SDL_QUIT:
                *running = false;
                break;
            case SDL_WINDOWEVENT:
                switch (windowEvent.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                    windowevent_resized = true;
                    break;
                case SDL_WINDOWEVENT_MAXIMIZED:
                    if (!windowFullScreen) {
                        windowMaximised = true;
                    }
                    break;
                case SDL_WINDOWEVENT_RESTORED:
                    if (!windowFullScreen) {
                        windowMaximised = false;
                        SDL_SetWindowSize(sdl_window, windowRestoredSize[0], windowRestoredSize[1]);
                        SDL_SetWindowPosition(sdl_window, windowRestoredPos[0], windowRestoredPos[1]);
                    }
                    break;
                }
                break;
            }
        }
        if (windowevent_resized) {
            SDL_GetWindowSize(sdl_window, &windowDimensions[0], &windowDimensions[1]);
            worldFrameBuffer.resize(windowDimensions);
            glBindTexture(GL_TEXTURE_2D, skyTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowDimensions[0], windowDimensions[1], 0, GL_RGBA, GL_FLOAT, NULL);
            glViewport(0, 0, windowDimensions[0], windowDimensions[1]);
            crosshairProj = glm::ortho(-(float)windowDimensions[0] / 2, (float)windowDimensions[0] / 2, -(float)windowDimensions[1] / 2, (float)windowDimensions[1] / 2, -1.0f, 1.0f);
            crosshairShader.setUniformMat4f("u_MVP", crosshairProj);
            windowFlags = SDL_GetWindowFlags(sdl_window);
            if (!((windowFlags & SDL_WINDOW_MAXIMIZED) || lastLastWindowFullScreen || windowFullScreen)) {
                windowRestoredSize[0] = windowDimensions[0];
                windowRestoredSize[1] = windowDimensions[1];
                SDL_GetWindowPosition(sdl_window, &windowRestoredPos[0], &windowRestoredPos[1]);
            }
        }
        lastLastWindowFullScreen = lastWindowFullScreen;
        lastWindowFullScreen = windowFullScreen;

        //render if a frame is needed
        end = std::chrono::steady_clock::now();
        double currentTime = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000;
        if (currentTime > frameStart + DT) {
            double actualDT = currentTime - frameStart;
            if (currentTime - lastFrameRateTime > 1) {
                std::cout << (frames - lastFrameRateFrames) / (1.0) << " FPS\n";
                std::cout << m_mainPlayer->viewCamera.position[0] + m_mainPlayer->cameraBlockPosition[0] << ", " << m_mainPlayer->viewCamera.position[1] + m_mainPlayer->cameraBlockPosition[1] << ", " << m_mainPlayer->viewCamera.position[2] + m_mainPlayer->cameraBlockPosition[2] << "\n";
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
            
            //create model view projection matrix for the world
            float fov = 70.0;
            fov = fov - fov * (2.0 / 3.0) * m_mainPlayer->zoom;
            glm::mat4 projection = glm::perspective(glm::radians(fov), ((float)windowDimensions[0] / (float)windowDimensions[1]), 0.12f, (float)((m_mainWorld->getRenderDistance() - 1) * constants::CHUNK_SIZE));
            glm::mat4 inverseProjection = glm::inverse(projection);
            glm::mat4 view;
            m_mainPlayer->viewCamera.getViewMatrix(&view);
            glm::mat4 inverseView = glm::inverse(view);
            glm::mat4 model = (glm::mat4(1.0f));
            glm::mat4 mvp = projection * view * model;
            //update the MVP uniform
            blockShader.bind();
            blockShader.setUniformMat4f("u_modelView", view * model);
            blockShader.setUniformMat4f("u_proj", projection);

            int breakBlockCoords[3];
            int placeBlockCoords[3];
            bool lookingAtBlock;
            if (m_mainWorld->shootRay(m_mainPlayer->viewCamera.position, m_mainPlayer->cameraBlockPosition, m_mainPlayer->viewCamera.front, breakBlockCoords, placeBlockCoords)) {
                //create the model view projection matrix for the outline
                glm::vec3 outlinePosition;
                for (unsigned char i = 0; i < 3; i++) {
                    outlinePosition[i] = breakBlockCoords[i] - m_mainPlayer->cameraBlockPosition[i];
                }
                model = glm::translate(model, outlinePosition);
                glm::mat4 mvp = projection * view * model;
                blockOutlineShader.bind();
                blockOutlineShader.setUniformMat4f("u_MVP", mvp);

                lookingAtBlock = true;
            }
            else {
                lookingAtBlock = false;
            }

            // Render sky
            glBindImageTexture(0, skyTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
            skyShader.bind();
            unsigned int timeOfDay = frames / 50 % constants::DAY_LENGTH;
            skyShader.setUniformVec3("sunDir", glm::vec3(glm::cos((float)timeOfDay /
                constants::DAY_LENGTH * glm::pi<float>() * 2), glm::sin((float)timeOfDay /
                constants::DAY_LENGTH * glm::pi<float>() * 2), 0.0f));
            skyShader.setUniformMat4f("inverseProjection", inverseProjection);
            skyShader.setUniformMat4f("inverseView", inverseView);
            glDispatchCompute((unsigned int)((windowDimensions[0] + 7) / 8),
              (unsigned int)((windowDimensions[1] + 7) / 8), 1);
            //glDispatchCompute((unsigned int)(windowDimensions[0]), (unsigned int)(windowDimensions[1]), 1);
            // Make sure writing to image has finished before read
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            // Render the world to a texture
            //worldFrameBuffer.bind();
            mainRenderer.clear();
            // Draw the sky
            screenShader.bind();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, skyTexture);
            screenVA.bind();
            glDrawArrays(GL_TRIANGLES, 0, 6);
            // Render the world geometry
            glEnable(GL_DEPTH_TEST);
            allBlockTextures.bind();
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, skyTexture);
            //auto tp1 = std::chrono::high_resolution_clock::now();
            m_mainWorld->renderChunks(mainRenderer, blockShader, waterShader, view, projection, m_mainPlayer->cameraBlockPosition, (float)windowDimensions[0] / (float)windowDimensions[1], fov, actualDT);
            //auto tp2 = std::chrono::high_resolution_clock::now();
            //cout << std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count() << "us\n";
            if (lookingAtBlock) {
                mainRenderer.drawWireframe(blockOutlineVA, blockOutlineIB, blockOutlineShader);
            }
            //worldFrameBuffer.unbind();

            // Draw the world texture
            //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            //glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            //worldFrameBuffer.draw(screenShader);
            // Draw the crosshair
            glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
            glActiveTexture(GL_TEXTURE0);
            mainRenderer.draw(crosshairVA, crosshairIB, crosshairShader);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            SDL_GL_SwapWindow(sdl_window);

            m_mainPlayer->processUserInput(sdl_window, windowDimensions, &windowLastFocus, running, currentTime, m_networking);
            float cameraPos[3];
            cameraPos[0] = m_mainPlayer->cameraBlockPosition[0] + m_mainPlayer->viewCamera.position[0];
            cameraPos[1] = m_mainPlayer->cameraBlockPosition[1] + m_mainPlayer->viewCamera.position[1];
            cameraPos[2] = m_mainPlayer->cameraBlockPosition[2] + m_mainPlayer->viewCamera.position[2];
            m_mainWorld->updatePlayerPos(cameraPos[0], cameraPos[1], cameraPos[2]);

            frames++;
        }
        m_mainWorld->updateMeshes();
        m_mainWorld->doRenderThreadJobs();

        loopRunning = *running;
        for (char i = 0; i < m_mainWorld->getNumChunkLoaderThreads(); i++) {
            loopRunning |= m_chunkLoaderThreadsRunning[i];
        }
    }

    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}

void RenderThread::renderSky() {

}

}  // namespace client