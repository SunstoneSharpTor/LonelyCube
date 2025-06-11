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

#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

#include "core/pch.h"

#include "client/applicationState.h"
#include "client/game.h"
#include "client/graphics/renderer.h"
#include "client/gui/menuUpdateInfo.h"
#include "client/gui/pauseMenu.h"
#include "client/gui/startMenu.h"
#include "client/input.h"
#include "core/config.h"
#include "core/log.h"

namespace lonelycube::client {

void renderThread()
{
    Renderer renderer(VK_SAMPLE_COUNT_4_BIT, 1.0f);
    ApplicationState applicationState;

    Config settings("res/settings.txt");
    uint32_t worldSeed;
    Game* game = nullptr;

    glm::ivec2 windowSize({
        renderer.getVulkanEngine().getSwapchainExtent().width,
        renderer.getVulkanEngine().getSwapchainExtent().height
    });
    StartMenu startMenu(windowSize);
    PauseMenu pauseMenu(windowSize);

    bool running = true;

    bool windowFullScreen = false;
    int windowDimensions[2];
    int smallScreenWindowDimensions[2];
    int smallScreenWindowPos[2];
    glfwGetWindowSize(
        renderer.getVulkanEngine().getWindow(), &windowDimensions[0], &windowDimensions[1]
    );
    const GLFWvidmode* displayMode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    // glfwSetCharCallback(renderer.getVulkanEngine().getWindow(), input::characterCallback);
    glfwSetKeyCallback(renderer.getVulkanEngine().getWindow(), input::keyCallback);
    glfwSetMouseButtonCallback(renderer.getVulkanEngine().getWindow(), input::mouseButtonCallback);

    //set up game loop
    float exposure = 0.0;
    float toneMapTimeByDTs = 0.0;
    int FPS = 0;
    int FPS_CAP = std::numeric_limits<int>::max();
    double DT = 1.0 / FPS_CAP;
    uint64_t lastFrameRateFrames = 0;
    auto start = std::chrono::steady_clock::now();
    auto end = start;
    double frameStart = -DT;
    float lastFrameRateTime = frameStart + DT;
    while (running)
    {
        glfwPollEvents();

        //toggle fullscreen if F11 pressed
        if (input::peekAtButtonPressed(glfwGetKeyScancode(GLFW_KEY_F11)))
        {
            if (windowFullScreen)
            {
                glfwSetWindowMonitor(
                    renderer.getVulkanEngine().getWindow(), nullptr, smallScreenWindowPos[0],
                    smallScreenWindowPos[1], smallScreenWindowDimensions[0],
                    smallScreenWindowDimensions[1], displayMode->refreshRate);
            }
            else
            {
                smallScreenWindowDimensions[0] = windowDimensions[0];
                smallScreenWindowDimensions[1] = windowDimensions[1];
                glfwGetWindowPos(
                    renderer.getVulkanEngine().getWindow(), &smallScreenWindowPos[0],
                    &smallScreenWindowPos[1]);
                glfwSetWindowMonitor(
                    renderer.getVulkanEngine().getWindow(), glfwGetPrimaryMonitor(), 0, 0,
                    displayMode->width, displayMode->height, displayMode->refreshRate);
            }

            windowFullScreen = !windowFullScreen;
        }

        glfwGetWindowSize(renderer.getVulkanEngine().getWindow(), &windowSize.x, &windowSize.y);
        if (windowDimensions[0] != windowSize.x || windowDimensions[1] != windowSize.y)
        {
            windowDimensions[0] = windowSize.x;
            windowDimensions[1] = windowSize.y;
        }

        if (renderer.getVulkanEngine().windowResized())
        {
            renderer.getVulkanEngine().recreateSwapchain();
            renderer.resize();
        }

        // Render if a frame is needed
        end = std::chrono::steady_clock::now();
        double currentTime = (double)std::chrono::duration_cast<std::chrono::microseconds>
            (end - start).count() / 1000000;
        if (currentTime > frameStart + DT)
        {
            double actualDT = currentTime - frameStart;
            // Update FPS counter
            if (currentTime - lastFrameRateTime > 1)
            {
                FPS = renderer.getVulkanEngine().getCurrentFrame() - lastFrameRateFrames;
                lastFrameRateTime += 1;
                lastFrameRateFrames = renderer.getVulkanEngine().getCurrentFrame();
            }
            // Update frame rate limiter
            if ((currentTime - DT) < (frameStart + DT))
                frameStart += DT;
            else
                frameStart = currentTime;

            if (renderer.beginRenderingFrame() && !renderer.isMinimised())
            {
                // Update GUI
                input::swapBuffers();

                double xPos, yPos;
                glfwGetCursorPos(renderer.getVulkanEngine().getWindow(), &xPos, &yPos);
                int mousePressed = glfwGetMouseButton(
                    renderer.getVulkanEngine().getWindow(), GLFW_MOUSE_BUTTON_LEFT
                );
                MenuUnpdateInfo menuUpdateInfo {
                    .applicationState = applicationState,
                    .game = game
                };
                menuUpdateInfo.guiScale = std::max(
                    1,
                    static_cast<int>(renderer.getVulkanEngine().getSwapchainExtent().height) / 217
                );
                menuUpdateInfo.windowSize = glm::ivec2(
                    renderer.getVulkanEngine().getSwapchainExtent().width,
                    renderer.getVulkanEngine().getSwapchainExtent().height
                );
                menuUpdateInfo.cursorPos = glm::ivec2(std::floor(xPos), std::floor(yPos));

                switch (applicationState.getState().back())
                {
                case ApplicationState::Gameplay:
                    if (input::buttonPressed(glfwGetKeyScancode(GLFW_KEY_ESCAPE)))
                    {
                        game->unfocus();
                        renderer.setGameBrightness(0.25f);
                        applicationState.pushState(ApplicationState::PauseMenu);
                        input::clearCurrentBuffer();
                    }
                    if (input::buttonPressed(glfwGetKeyScancode(GLFW_KEY_F3)))
                    {
                        applicationState.toggleDebugInfo();
                    }
                    break;

                case ApplicationState::StartMenu:
                    if (startMenu.update(menuUpdateInfo) && !applicationState.getState().empty()
                        && applicationState.getState().back() == ApplicationState::Gameplay)
                    {
                        worldSeed = std::time(nullptr);
                        LOG("World Seed: " + std::to_string(worldSeed));
                        game = new Game(
                            renderer, settings.getMultiplayer(), settings.getServerIP(),
                            settings.getRenderDistance(), worldSeed
                        );
                        game->focus();
                        renderer.setGameBrightness(1.0f);
                    }
                    input::clearCurrentBuffer();
                    break;

                case ApplicationState::PauseMenu:
                    if (pauseMenu.update(menuUpdateInfo)
                        && applicationState.getState().back() == ApplicationState::Gameplay)
                    {
                        renderer.setGameBrightness(1.0f);
                    }
                    input::clearCurrentBuffer();
                    break;

                default:
                    break;
                }

                if (applicationState.getState().empty())
                    break;

                // Update and draw game
                if (std::count(
                        applicationState.getState().begin(),
                        applicationState.getState().end(), ApplicationState::Gameplay
                )) {
                    game->processInput(actualDT);
                    game->renderFrame(actualDT);
                    bool gameplayFocused = applicationState.getState().back() == ApplicationState::Gameplay;
                    if (applicationState.isDebugInfoEnabled())
                        game->queueDebugText(menuUpdateInfo.guiScale, FPS);
                }
                else
                {
                    renderer.beginRenderingToSwapchainImage();
                    renderer.drawBackgroundImage();
                }

                // Draw GUI
                switch (applicationState.getState().back())
                {
                case ApplicationState::StartMenu:
                    renderer.menuRenderer.queue(startMenu.getMenu());
                    break;
                case ApplicationState::PauseMenu:
                    renderer.menuRenderer.queue(pauseMenu.getMenu());
                    break;

                default:
                    break;
                }

                renderer.beginDrawingUi();
                renderer.menuRenderer.draw();
                renderer.font.draw();

                renderer.submitFrame();
            }
        }

        if (std::count(
            applicationState.getState().begin(),
            applicationState.getState().end(), ApplicationState::Gameplay))
        {
            game->getWorld().unloadOutOfRangeMeshesIfNeeded();
            game->getWorld().doRenderThreadJobs();
        }

        if (glfwWindowShouldClose(renderer.getVulkanEngine().getWindow()))
        {
            if (game != nullptr)
                delete game;

            running = false;
        }
    }
}

}  // namespace lonelycube::client
