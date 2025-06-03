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

#include "client/game.h"

#include <enet/enet.h>
#include "glm/fwd.hpp"

#include "core/pch.h"

#include "client/logicThread.h"
#include "client/graphics/camera.h"
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

Game::Game(
    Renderer& renderer, bool multiplayer, const std::string& serverIP, int renderDistance,
    uint64_t worldSeed
) : m_multiplayer(multiplayer),
    m_running(true),
    m_renderer(renderer),
    m_mainWorld(
        renderDistance, worldSeed, !m_multiplayer, { 0, 200, 0 },
        m_multiplayer ? m_networking.getPeer() : nullptr, m_networking.getMutex(), m_renderer
    ),
    m_mainPlayer({ 0, 200, 0 }, &m_mainWorld, m_mainWorld.integratedServer.getResourcePack()),
    m_chunkLoaderThreadsRunning(m_mainWorld.getNumChunkLoaderThreads())
{
    if (m_multiplayer) {
        if (!m_networking.establishConnection(serverIP, renderDistance))
            m_multiplayer = false;
    }

    std::fill(
        m_chunkLoaderThreadsRunning.begin(), m_chunkLoaderThreadsRunning.end(), true
    );

    LogicThread logicThread(
        m_mainWorld, m_chunkLoaderThreadsRunning, m_mainPlayer, m_networking, m_multiplayer
    );
    m_logicWorker = std::thread(&LogicThread::go, logicThread, std::ref(m_running));

    m_mainWorld.updatePlayerPos(
        m_mainPlayer.cameraBlockPosition, &(m_mainPlayer.viewCamera.position[0])
    );

    int windowDimensions[2];
    m_mainPlayer.processUserInput(
        renderer.getVulkanEngine().getWindow(), windowDimensions, 0.0, m_networking
    );
    m_mainWorld.doRenderThreadJobs();
}

void Game::processInput(double dt)
{
    VkExtent2D windowDimensions = m_renderer.getVulkanEngine().getSwapchainExtent();
    m_mainPlayer.processUserInput(
        m_renderer.getVulkanEngine().getWindow(), reinterpret_cast<int*>(&windowDimensions.width),
        dt, m_networking
    );
}

void Game::focus()
{
    VkExtent2D windowDimensions = m_renderer.getVulkanEngine().getSwapchainExtent();
    m_mainPlayer.focus(
        m_renderer.getVulkanEngine().getWindow(), reinterpret_cast<int*>(&windowDimensions.width)
    );
}

void Game::unfocus()
{
    VkExtent2D windowDimensions = m_renderer.getVulkanEngine().getSwapchainExtent();
    m_mainPlayer.unfocus(
        m_renderer.getVulkanEngine().getWindow(), reinterpret_cast<int*>(&windowDimensions.width)
    );
}

void Game::renderFrame(double dt)
{
    m_mainWorld.updateMeshes();
    m_mainWorld.updatePlayerPos(
        m_mainPlayer.cameraBlockPosition, &(m_mainPlayer.viewCamera.position[0])
    );

    // Create model view projection matrices for the world
    float fov = 60.0f;
    fov = fov - fov * (2.0f / 3) * m_mainPlayer.zoom;
    VkExtent2D windowDimensions = m_renderer.getVulkanEngine().getSwapchainExtent();
    glm::mat4 projection = glm::perspective(
        glm::radians(fov), ((float)windowDimensions.width / windowDimensions.height), 0.1f,
        (float)((m_mainWorld.getRenderDistance() - 1) * constants::CHUNK_SIZE)
    );
    glm::mat4 projectionReversedDepth = glm::perspective(
        glm::radians(fov), ((float)windowDimensions.width / windowDimensions.height),
        (float)((m_mainWorld.getRenderDistance() - 1) * constants::CHUNK_SIZE), 0.1f
    );
    glm::mat4 view;
    m_mainPlayer.viewCamera.getViewMatrix(&view);
    glm::mat4 viewProjection = projectionReversedDepth * view;

    uint32_t timeOfDay =
        (m_mainWorld.integratedServer.getTickNum() + constants::DAY_LENGTH / 4) %
        constants::DAY_LENGTH;
    float groundLuminance = calculateBrightness(
        constants::GROUND_LUMINANCE, constants::NUM_GROUND_LUMINANCE_POINTS, timeOfDay
    );
    // LOG(std::to_string(timeOfDay) + ": " + std::to_string(groundLuminance));

    glm::vec3 sunDirection(glm::cos((float)((timeOfDay + constants::DAY_LENGTH * 3 / 4) % constants::DAY_LENGTH) /
        constants::DAY_LENGTH * glm::pi<float>() * 2), glm::sin((float)((timeOfDay + constants::DAY_LENGTH * 3 / 4) % constants::DAY_LENGTH) /
        constants::DAY_LENGTH * glm::pi<float>() * 2), 0.0f);
    m_renderer.skyRenderInfo.sunDir = sunDirection;
    m_renderer.skyRenderInfo.inverseViewProjection = glm::inverse(
        projection * glm::lookAt(
            glm::vec3(0.0f), m_mainPlayer.viewCamera.front, -m_mainPlayer.viewCamera.up
    ));
    m_renderer.skyRenderInfo.brightness = groundLuminance;
    m_renderer.skyRenderInfo.sunGlowColour = glm::vec3(1.7f, 0.67f, 0.13f);
    m_renderer.skyRenderInfo.sunGlowAmount = std::pow(
        std::abs(glm::dot(sunDirection, glm::vec3(1.0f, 0.0f, 0.0f))), 64.0f
    );

    m_renderer.drawSky();
    m_mainWorld.buildEntityMesh(m_mainPlayer.cameraBlockPosition);
    m_renderer.beginDrawingGeometry();
    m_renderer.blitSky();

    // Render the world geometry
    float cameraSubBlockPos[3];
    m_mainPlayer.viewCamera.getPosition(cameraSubBlockPos);
    FrameData& currentFrameData = m_renderer.getVulkanEngine().getCurrentFrameData();
    VkCommandBuffer command = currentFrameData.commandBuffer;
    #ifdef TIMESTAMPS
    vkCmdWriteTimestamp(
        command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, renderer.getVulkanEngine().getCurrentTimestampQueryPool(), 0
    );
    #endif
    m_mainWorld.renderWorld(
        viewProjection, m_mainPlayer.cameraBlockPosition,
        glm::vec3(cameraSubBlockPos[0], cameraSubBlockPos[1], cameraSubBlockPos[2]),
        (float)windowDimensions.width / (float)windowDimensions.height, fov, groundLuminance,
        dt
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
    uint8_t lookingAtBlock = m_mainWorld.shootRay(
        m_mainPlayer.viewCamera.position, m_mainPlayer.cameraBlockPosition,
        m_mainPlayer.viewCamera.front, breakBlockCoords, placeBlockCoords
    );
    if (lookingAtBlock)
    {
        glm::vec3 offset;
        for (uint8_t i = 0; i < 3; i++)
            offset[i] = breakBlockCoords[i] - m_mainPlayer.cameraBlockPosition[i];
        offset += glm::vec3(0.5f, 0.5f, 0.5f);
        m_renderer.drawBlockOutline(
            viewProjection, offset,
            m_mainWorld.integratedServer.getResourcePack().getBlockData(
                lookingAtBlock).model->boundingBoxVertices
        );
    }

    m_renderer.finishDrawingGeometry();
    m_renderer.renderBloom();
    m_renderer.calculateAutoExposure(dt);
    m_renderer.beginRenderingToSwapchainImage();
    m_renderer.applyToneMap();
    m_renderer.drawCrosshair();
}

Game::~Game()
{
    do
    {
        m_running = false;
        for (int8_t i = 0; i < m_mainWorld.getNumChunkLoaderThreads(); i++) {
            m_running |= m_chunkLoaderThreadsRunning[i];
        }
        m_mainWorld.doRenderThreadJobs();
    } while (m_running);

    vkDeviceWaitIdle(m_renderer.getVulkanEngine().getDevice());
    m_mainWorld.unloadAllMeshes();
    m_mainWorld.freeEntityMeshes();

    m_logicWorker.join();

    if (m_multiplayer) {
        m_networking.disconnect(m_mainWorld);
        enet_deinitialize();
    }
}

}  // namespace lonelycube::client
