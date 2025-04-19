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

#include "glm/glm.hpp"

#include "core/pch.h"

#include "client/applicationStateManager.h"
#include "client/game.h"
#include "client/graphics/renderer.h"
#include "client/gui/menuUpdateInfo.h"
#include "client/gui/startMenu.h"
#include "core/config.h"
#include "core/log.h"

namespace lonelycube::client {

static std::string testText;

void characterCallback(GLFWwindow* window, unsigned int codepoint)
{
    testText = testText + (char)codepoint;
}

void renderThread()
{
    Renderer renderer(VK_SAMPLE_COUNT_4_BIT, 1.0f);
    ApplicationStateManager applicationStateManager;

    Config settings("res/settings.txt");
    uint32_t worldSeed = std::time(nullptr);
    LOG("World Seed: " + std::to_string(worldSeed));
    Game game(
        renderer, settings.getMultiplayer(), settings.getServerIP(), settings.getRenderDistance(),
        worldSeed
    );

    StartMenu startMenu({
        renderer.getVulkanEngine().getSwapchainExtent().width,
        renderer.getVulkanEngine().getSwapchainExtent().height
    });

    bool running = true;

    bool windowLastFocus = false;
    bool windowFullScreen = false;
    bool lastWindowFullScreen = false;
    bool lastLastWindowFullScreen = false;
    int windowRestoredSize[2];
    int windowRestoredPos[2];
    bool lastF11 = false;
    bool lastMousePressed = true;
    int windowDimensions[2];
    int smallScreenWindowDimensions[2];
    int smallScreenWindowPos[2];
    glfwGetWindowSize(
        renderer.getVulkanEngine().getWindow(), &windowDimensions[0], &windowDimensions[1]
    );
    const GLFWvidmode* displayMode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    // glfwSetCharCallback(renderer.getVulkanEngine().getWindow(), characterCallback);

    //set up game loop
    float exposure = 0.0;
    float toneMapTimeByDTs = 0.0;
    int FPS_CAP = std::numeric_limits<int>::max();
    double DT = 1.0 / FPS_CAP;
    uint64_t lastFrameRateFrames = 0;
    auto start = std::chrono::steady_clock::now();
    auto end = start;
    double frameStart = -DT;
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
                        game.getPlayer().viewCamera.position[0] + game.getPlayer().cameraBlockPosition[0]
                    ) + ", " + std::to_string(
                        game.getPlayer().viewCamera.position[1] + game.getPlayer().cameraBlockPosition[1]
                    ) + ", " + std::to_string(
                        game.getPlayer().viewCamera.position[2] + game.getPlayer().cameraBlockPosition[2]
                    )
                );
                lastFrameRateTime += 1;
                lastFrameRateFrames = renderer.getVulkanEngine().getCurrentFrame();
            }
            // Update frame rate limiter
            if ((currentTime - DT) < (frameStart + DT))
                frameStart += DT;
            else
                frameStart = currentTime;

            renderer.beginRenderingFrame();
            if (!renderer.isMinimised())
            {
                // Update GUI
                double xPos, yPos;
                glfwGetCursorPos(renderer.getVulkanEngine().getWindow(), &xPos, &yPos);
                int mousePressed = glfwGetMouseButton(
                    renderer.getVulkanEngine().getWindow(), GLFW_MOUSE_BUTTON_LEFT
                );
                MenuUnpdateInfo menuUpdateInfo {
                    .applicationStateManager = applicationStateManager,
                    .game = game
                };
                menuUpdateInfo.mouseClicked = mousePressed == GLFW_PRESS && !lastMousePressed;
                lastMousePressed = mousePressed == GLFW_PRESS;
                menuUpdateInfo.guiScale = std::max(
                    1,
                    static_cast<int>(renderer.getVulkanEngine().getSwapchainExtent().height) / 217
                );
                menuUpdateInfo.windowSize = glm::ivec2(
                    renderer.getVulkanEngine().getSwapchainExtent().width,
                    renderer.getVulkanEngine().getSwapchainExtent().height
                );
                menuUpdateInfo.cursorPos = glm::ivec2(std::floor(xPos), std::floor(yPos));

                switch (applicationStateManager.getState().back())
                {
                case ApplicationStateManager::StartMenu:
                    startMenu.update(menuUpdateInfo);
                    break;

                default:
                    break;
                }
                if (applicationStateManager.getState().empty())
                    break;

                // Update and draw game
                if (std::find(
                        applicationStateManager.getState().begin(),
                        applicationStateManager.getState().end(), ApplicationStateManager::Gameplay
                    ) != applicationStateManager.getState().end()
                ) {
                    game.renderFrame(currentTime, actualDT);
                }
                else
                {
                    renderer.beginRenderingToSwapchainImage();
                }

                // Draw GUI
                switch (applicationStateManager.getState().back())
                {
                case ApplicationStateManager::StartMenu:
                    renderer.menuRenderer.queue(startMenu.getMenu());
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

        game.getWorld().doRenderThreadJobs();

        if (glfwWindowShouldClose(renderer.getVulkanEngine().getWindow()))
            running = false;
    }
}

}  // namespace lonelycube::client
