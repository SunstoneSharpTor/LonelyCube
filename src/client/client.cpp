/*
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

#include <iostream>
#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <enet/enet.h>
#include <fstream>
#include <string>
#include <sstream>
#include <time.h>
#include <chrono>
#include <thread>
#include <algorithm>

#include "client/renderer.h"
#include "client/vertexBuffer.h"
#include "client/indexBuffer.h"
#include "client/vertexArray.h"
#include "client/shader.h"
#include "client/texture.h"
#include "client/camera.h"
#include "core/chunk.h"
#include "client/clientWorld.h"
#include "client/player.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/noise.hpp"

using namespace client;

void chunkLoaderThread(ClientWorld* mainWorld, bool* running, char threadNum) {
    while (*running) {
        mainWorld->loadChunksAroundPlayer(threadNum);
    }
}

void networking(ENetHost* client) {
    ENetEvent event;
    while(enet_host_service(client, &event, 0) > 0) {
        switch(event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                std::cout << "A packet of length " << event.packet->dataLength
                    << " containing " << event.packet->data
                    << " was received from " << event.peer->address.host
                    << ":" << event.peer->address.port
                    << " on channel " << event.channelID << "\n";
                enet_packet_destroy(event.packet);
                break;
        }
    }
}

void renderThread(ClientWorld* mainWorld, bool* running, bool* chunkLoaderThreadsRunning, Player* mainPlayer) {
    const int defaultWindowDimensions[2] = { 853, 480 };
    int windowDimensions[2] = { defaultWindowDimensions[0], defaultWindowDimensions[1] };

    SDL_Window* sdl_window;
    SDL_GLContext context;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    sdl_window = SDL_CreateWindow(
        "Minecraft",
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

    mainPlayer->setWorldMouseData(sdl_window, windowDimensions);

    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

    //water shader
    Shader waterShader("shaders/basicVertex.txt", "shaders/waterFragment.txt");
    waterShader.bind();
    waterShader.setUniform1i("u_texture", 0);
    waterShader.setUniform1f("u_renderDistance", (mainWorld->getWorldInfo().renderDistance - 1) * constants::CHUNK_SIZE);

    //set up the shader and the uniforms
    Shader blockShader("shaders/basicVertex.txt", "shaders/basicFragment.txt");
    blockShader.bind();

    Texture allBlockTextures("blockTextures.png");
    allBlockTextures.bind();
    blockShader.setUniform1i("u_texture", 0);
    blockShader.setUniform1f("u_renderDistance", (mainWorld->getWorldInfo().renderDistance - 1) * constants::CHUNK_SIZE);

    glClearColor(0.57f, 0.70f, 1.0f, 1.0f);

    Renderer mainRenderer;

    mainRenderer.setOpenGlOptions();

    //set up crosshair
    float crosshairCoordinates[24] = { -1.0f,  8.0f,
                                        1.0f,  8.0f,
                                        1.0f, -8.0f,
                                       -1.0f, -8.0f,

                                       -8.0f,  1.0f,
                                        8.0f,  1.0f,
                                        8.0f, -1.0f,
                                       -8.0f, -1.0f };

    unsigned int crosshairIndices[12] = { 2, 1, 0,
                                          0, 3, 2,
                                          6, 5, 4,
                                          4, 7, 6 };

    VertexArray crosshairVA;
    VertexBuffer crosshairVB(crosshairCoordinates, 24 * sizeof(float));
    VertexBufferLayout crosshairVBlayout;
    crosshairVBlayout.push<float>(2);
    crosshairVA.addBuffer(crosshairVB, crosshairVBlayout);
    IndexBuffer crosshairIB(crosshairIndices, 12);

    Shader crosshairShader("shaders/crosshairVertex.txt", "shaders/crosshairFragment.txt");
    crosshairShader.bind();
    glm::mat4 crosshairProj = glm::ortho(-(float)windowDimensions[0] / 2, (float)windowDimensions[0] / 2, -(float)windowDimensions[1] / 2, (float)windowDimensions[1] / 2, -1.0f, 1.0f);
    crosshairShader.setUniformMat4f("u_MVP", crosshairProj);

    //set up block outline
    VertexArray blockOutlineVA;
    VertexBuffer blockOutlineVB(constants::WIREFRAME_CUBE_FACE_POSITIONS, 24 * sizeof(float));
    VertexBufferLayout blockOutlineVBL;
    blockOutlineVBL.push<float>(3);
    blockOutlineVA.addBuffer(blockOutlineVB, blockOutlineVBL);
    IndexBuffer blockOutlineIB(constants::CUBE_WIREFRAME_IB, 16);

    Shader blockOutlineShader("shaders/wireframeVertex.txt", "shaders/wireframeFragment.txt");

    float cameraPos[3];
    cameraPos[0] = mainPlayer->cameraBlockPosition[0] + mainPlayer->viewCamera.position[0];
    cameraPos[1] = mainPlayer->cameraBlockPosition[1] + mainPlayer->viewCamera.position[1];
    cameraPos[2] = mainPlayer->cameraBlockPosition[2] + mainPlayer->viewCamera.position[2];
    mainWorld->initPlayerPos(cameraPos[0], cameraPos[1], cameraPos[2]);
    
    auto start = std::chrono::steady_clock::now();
    auto end = std::chrono::steady_clock::now();
    double time = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000;
    mainPlayer->processUserInput(sdl_window, windowDimensions, &windowLastFocus, running, time);
    mainWorld->doRenderThreadJobs();

    //set up game loop
    SDL_DisplayMode displayMode;
    SDL_GetWindowDisplayMode(sdl_window, &displayMode);
    int FPS_CAP = (displayMode.refresh_rate) * VSYNC + 100000 * (!VSYNC);
    double DT = 1.0 / FPS_CAP;
    long frames = 0;
    long lastFrameRateFrames = 0;
    end = std::chrono::steady_clock::now();
    time = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000;
    double frameStart = time - DT;
    bool windowevent_resized = false;
    float lastFrameRateTime = frameStart + DT;
    bool loopRunning = *running;
    while (loopRunning) {
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
                std::cout << mainPlayer->viewCamera.position[0] + mainPlayer->cameraBlockPosition[0] << ", " << mainPlayer->viewCamera.position[1] + mainPlayer->cameraBlockPosition[1] << ", " << mainPlayer->viewCamera.position[2] + mainPlayer->cameraBlockPosition[2] << "\n";
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
            float FOV = 70.0;
            FOV = FOV - FOV * (2.0 / 3.0) * mainPlayer->zoom;
            glm::mat4 proj = glm::perspective(glm::radians(FOV), ((float)windowDimensions[0] / (float)windowDimensions[1]), 0.12f, static_cast<float>((mainWorld->getWorldInfo().renderDistance - 1) * constants::CHUNK_SIZE));
            glm::mat4 view;
            mainPlayer->viewCamera.getViewMatrix(&view);
            glm::mat4 model = (glm::mat4(1.0f));
            glm::mat4 mvp = proj * view * model;
            //update the MVP uniform
            blockShader.bind();
            blockShader.setUniformMat4f("u_modelView", view * model);
            blockShader.setUniformMat4f("u_proj", proj);

            int breakBlockCoords[3];
            int placeBlockCoords[3];
            bool lookingAtBlock;
            if (mainWorld->shootRay(mainPlayer->viewCamera.position, mainPlayer->cameraBlockPosition, mainPlayer->viewCamera.front, breakBlockCoords, placeBlockCoords)) {
                //create the model view projection matrix for the outline
                glm::vec3 outlinePosition;
                for (unsigned char i = 0; i < 3; i++) {
                    outlinePosition[i] = breakBlockCoords[i] - mainPlayer->cameraBlockPosition[i];
                }
                model = glm::translate(model, outlinePosition);
                glm::mat4 mvp = proj * view * model;
                blockOutlineShader.bind();
                blockOutlineShader.setUniformMat4f("u_MVP", mvp);

                lookingAtBlock = true;
            }
            else {
                lookingAtBlock = false;
            }

            //draw background
            mainRenderer.clear();

            //auto tp1 = std::chrono::high_resolution_clock::now();
            mainWorld->renderChunks(mainRenderer, blockShader, waterShader, view, proj, mainPlayer->cameraBlockPosition, (float)windowDimensions[0] / (float)windowDimensions[1], FOV, actualDT);
            //auto tp2 = std::chrono::high_resolution_clock::now();
            //cout << std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count() << "us\n";

            if (lookingAtBlock) {
                mainRenderer.drawWireframe(blockOutlineVA, blockOutlineIB, blockOutlineShader);
            }

            glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
            mainRenderer.draw(crosshairVA, crosshairIB, crosshairShader);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            SDL_GL_SwapWindow(sdl_window);

            mainPlayer->processUserInput(sdl_window, windowDimensions, &windowLastFocus, running, currentTime);
            float cameraPos[3];
            cameraPos[0] = mainPlayer->cameraBlockPosition[0] + mainPlayer->viewCamera.position[0];
            cameraPos[1] = mainPlayer->cameraBlockPosition[1] + mainPlayer->viewCamera.position[1];
            cameraPos[2] = mainPlayer->cameraBlockPosition[2] + mainPlayer->viewCamera.position[2];
            mainWorld->updatePlayerPos(cameraPos[0], cameraPos[1], cameraPos[2]);

            frames++;
        }
        mainWorld->doRenderThreadJobs();

        loopRunning = *running;
        for (char i = 0; i < mainWorld->getNumChunkLoaderThreads(); i++) {
            loopRunning |= chunkLoaderThreadsRunning[i];
        }
    }

    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    bool MULTIPLAYER = false;
    
    ENetHost* client;
    ENetPeer* peer;
    ENetEvent event;

    if (MULTIPLAYER) {
        if (enet_initialize() != 0) {
            return EXIT_FAILURE;
        }
        
        client = enet_host_create(NULL, 1, 1, 0, 0);

        if (client == NULL) {
            return EXIT_FAILURE;
        }

        ENetAddress address;

        enet_address_set_host(&address, "127.0.0.1");
        address.port = 5555;

        peer = enet_host_connect(client, &address, 1, 0);
        if (peer == NULL) {
            return EXIT_FAILURE;
        }

        if ((enet_host_service(client, &event, 2000) > 0) && (event.type == ENET_EVENT_TYPE_CONNECT)) {
            std::cout << "Connection to 127.0.0.1 succeeded!" << std::endl;
        }
        else {
            enet_peer_reset(peer);
            std::cout << "Connection to 127.0.0.1 failed." << std::endl;
        }

        ENetPacket* packet = enet_packet_create((void*)"Hello World!", strlen("Hello World!") + 1, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(peer, 0, packet);
    }

    unsigned int worldSeed = std::time(0);
    ClientWorld mainWorld(32, worldSeed, !MULTIPLAYER, peer, client);
    std::cout << "World Seed: " << worldSeed << std::endl;
    int playerSpawnPoint[3] = { 0, 200, 0 };
    Player mainPlayer(playerSpawnPoint, &mainWorld);

    bool running = true;

    bool* chunkLoaderThreadsRunning = new bool[mainWorld.getNumChunkLoaderThreads()];
    std::fill(chunkLoaderThreadsRunning, chunkLoaderThreadsRunning + mainWorld.getNumChunkLoaderThreads(), true);

    std::thread renderWorker(renderThread, &mainWorld, &running, chunkLoaderThreadsRunning, &mainPlayer);

    std::thread* chunkLoaderThreads = new std::thread[mainWorld.getNumChunkLoaderThreads() - 1];
    for (char threadNum = 1; threadNum < mainWorld.getNumChunkLoaderThreads(); threadNum++) {
        chunkLoaderThreads[threadNum - 1] = std::thread(chunkLoaderThread, &mainWorld, &running, threadNum);
    }

    while (running) {
        mainWorld.loadChunksAroundPlayer(0);
        if (MULTIPLAYER) {
            networking(client);
        }
    }
    chunkLoaderThreadsRunning[0] = false;

    for (char threadNum = 1; threadNum < mainWorld.getNumChunkLoaderThreads(); threadNum++) {
        chunkLoaderThreads[threadNum - 1].join();
        chunkLoaderThreadsRunning[threadNum] = false;
    }
    renderWorker.join();

    delete chunkLoaderThreadsRunning;

    if (MULTIPLAYER) {
        enet_peer_disconnect(peer, 0);

        while (enet_host_service(client, &event, 3000) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
                case ENET_EVENT_TYPE_DISCONNECT:
                std::cout << "Disconnection succeeded!" << std::endl;
                break;
            }
        }

        enet_deinitialize();
    }

    return 0;
}