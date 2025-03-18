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

#include "client/clientWorld.h"

#include "client/graphics/renderer.h"
#include "client/graphics/vulkan/vulkanEngine.h"
#include "core/pch.h"

#include "client/graphics/camera.h"
#include "client/graphics/meshBuilder.h"
#include "core/constants.h"
#include "core/lighting.h"
#include "core/log.h"
#include "core/random.h"
#include "core/serverWorld.h"
#include "core/utils/iVec3.h"
#include "glm/ext/matrix_transform.hpp"

namespace lonelycube::client {

static bool chunkMeshUploaded[32] = { false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false };
static bool unmeshCompleted = true;

ClientWorld::ClientWorld(
    uint16_t renderDistance, uint64_t seed, bool singleplayer, const IVec3& playerPos,
    ENetPeer* peer, std::mutex& networkingMutex, Renderer& renderer
) : integratedServer(seed, networkingMutex), m_singleplayer(singleplayer), m_renderer(renderer),
    m_peer(peer), m_networkingMtx(networkingMutex), m_clientID(-1), m_chunkRequestScheduled(true),
    m_entityMeshManager(integratedServer)
{
    m_renderDistance = renderDistance + 1;
    m_renderDiameter = m_renderDistance * 2 + 1;
    m_meshedChunksDistance = 0.0f;
    m_fogDistance = 0.0f;
    m_timeByDTs = 0.0;
    m_renderingFrame = false;
    m_renderThreadWaitingForMeshUpdates = false;

    IVec3 playerChunkPosition = Chunk::getChunkCoords(playerPos);
    m_playerChunkPosition[0] = playerChunkPosition.x;
    m_playerChunkPosition[1] = playerChunkPosition.y;
    m_playerChunkPosition[2] = playerChunkPosition.z;

    float minUnloadedChunkDistance = ((m_renderDistance + 1) * (m_renderDistance + 1));

    m_numChunkLoadingThreads = integratedServer.getNumChunkLoaderThreads() - 1;

    //allocate arrays on the heap for the mesh to be built
    //do this now so that the same array can be reused for each chunk
    m_numChunkVertices = new uint32_t[m_numChunkLoadingThreads];
    m_numChunkIndices = new uint32_t[m_numChunkLoadingThreads];
    m_numChunkWaterVertices = new uint32_t[m_numChunkLoadingThreads];
    m_numChunkWaterIndices = new uint32_t[m_numChunkLoadingThreads];
    m_chunkVertices = new float*[m_numChunkLoadingThreads];
    m_chunkIndices = new uint32_t*[m_numChunkLoadingThreads];
    m_chunkWaterVertices = new float*[m_numChunkLoadingThreads];
    m_chunkWaterIndices = new uint32_t*[m_numChunkLoadingThreads];
    m_chunkPosition = new IVec3[m_numChunkLoadingThreads];
    m_chunkMeshReady = new bool[m_numChunkLoadingThreads];
    m_chunkMeshReadyCV = new std::condition_variable[m_numChunkLoadingThreads];
    m_chunkMeshReadyMtx = new std::mutex[m_numChunkLoadingThreads];
    m_threadWaiting = new bool[m_numChunkLoadingThreads];
    m_unmeshNeeded = false;

    m_entityMeshes.reserve(VulkanEngine::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < VulkanEngine::MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_entityMeshes.push_back(m_renderer.getVulkanEngine().allocateDynamicMesh(
            1680000 * sizeof(float), 360000 * sizeof(uint32_t)
        ));
    }

    for (int i = 0; i < m_numChunkLoadingThreads; i++) {
        m_chunkVertices[i] = new float[12 * 6 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        m_chunkIndices[i] = new uint32_t[18 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        m_chunkWaterVertices[i] = new float[12 * 6 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        m_chunkWaterIndices[i] = new uint32_t[18 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        m_chunkMeshReady[i] = false;
        m_threadWaiting[i] = false;
    }
    int playerBlockPosition[3] = { playerPos.x, playerPos.y, playerPos.z };
    float playerSubBlockPosition[3] = { 0.0f, 0.0f, 0.0f };
    integratedServer.addPlayer(playerBlockPosition, playerSubBlockPosition, m_renderDistance, !singleplayer);

    int i = 0;
    for (int x = -1; x < 2; x++) {
        for (int y = -1; y < 2; y++) {
            for (int z = -1; z < 2; z++) {
                m_neighbouringChunkIncludingDiaganalOffsets[i] = { x, y, z };
                i++;
            }
        }
    }
}

void ClientWorld::renderWorld(
    const glm::mat4& viewProj, const int* playerBlockPos, const glm::vec3& playerSubBlockPos,
    float aspectRatio, float fov, float skyLightIntensity, double DT
) {
    unloadMeshes();

    Frustum viewFrustum = m_viewCamera.createViewFrustum(aspectRatio, fov, 0, 20);
    m_renderingFrame = true;

    m_renderer.blockRenderInfo.skyLightIntensity = skyLightIntensity;
    m_timeByDTs += DT;
    while (m_timeByDTs > (1.0/(double)constants::visualTPS)) {
        double fac = 0.006;
        double targetDistance = sqrt(m_meshedChunksDistance) - 1.2;
        m_fogDistance = m_fogDistance * (1.0 - fac) + targetDistance * fac * constants::CHUNK_SIZE;
        m_timeByDTs -= (1.0/(double)constants::visualTPS);
    }
    m_renderer.blockRenderInfo.renderDistance = m_fogDistance;

    if (m_meshUpdates.size() > 0)
    {
        // auto tp1 = std::chrono::high_resolution_clock::now();
        while (m_meshUpdates.size() > 0)
        {
            m_meshUpdatesMtx.lock();
            for (const auto& chunk : m_meshUpdates)
            {
                if (!integratedServer.chunkManager.chunkLoaded(chunk))
                {
                    LOG("Updated chunk unloaded");
                }
            }
            m_meshUpdatesMtx.unlock();
            doRenderThreadJobs();
        }
        // auto tp2 = std::chrono::high_resolution_clock::now();
        // LOG("waited " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count()) + "us for chunks to remesh");
    }

    // Render Blocks
    m_renderer.beginDrawingBlocks();
    for (const auto& [chunkPosition, mesh] : m_meshes) {
        if (mesh.blockMesh.indexCount > 0) {
            IVec3 chunkCoordinates = chunkPosition * constants::CHUNK_SIZE - playerBlockPos;
            glm::vec3 coordinatesVec(chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
            AABB aabb(coordinatesVec, coordinatesVec + glm::vec3(constants::CHUNK_SIZE));
            if (aabb.isOnFrustum(viewFrustum)) {
                glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), coordinatesVec);
                m_renderer.blockRenderInfo.mvp = viewProj * modelMatrix;
                m_renderer.blockRenderInfo.cameraOffset = coordinatesVec - playerSubBlockPos;
                m_renderer.drawBlocks(mesh.blockMesh);
            }
            doRenderThreadJobs();
        }
    }

    // Render entities
    float timeSinceLastTick = integratedServer.getTimeSinceLastTick();
    integratedServer.getEntityManager().getPhysicsEngine().extrapolateTransforms(timeSinceLastTick);
    GPUDynamicMeshBuffers& entityMesh = m_entityMeshes[
        m_renderer.getVulkanEngine().getFrameDataIndex()
    ];
    m_entityMeshManager.createBatch(
        playerBlockPos, reinterpret_cast<float*>(entityMesh.vertexBuffer.mappedData),
        reinterpret_cast<uint32_t*>(entityMesh.indexBuffer.mappedData), timeSinceLastTick
    );
    // glm::vec3 coordinatesVec(-playerBlockPos[0], -playerBlockPos[1], -playerBlockPos[2]);
    m_renderer.blockRenderInfo.mvp = viewProj;  // * glm::translate(glm::mat4(1.0f), coordinatesVec);
    m_renderer.blockRenderInfo.cameraOffset = -playerSubBlockPos;
    m_renderer.drawEntities(m_entityMeshManager, entityMesh);

    // Render water
    m_renderer.beginDrawingWater();
    for (const auto& [chunkPosition, mesh] : m_meshes) {
        if (mesh.waterMesh.indexCount > 0) {
            IVec3 chunkCoordinates = chunkPosition * constants::CHUNK_SIZE - playerBlockPos;
            glm::vec3 coordinatesVec(chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
            AABB aabb(coordinatesVec, coordinatesVec + glm::vec3(constants::CHUNK_SIZE));
            if (aabb.isOnFrustum(viewFrustum)) {
                glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), coordinatesVec);
                m_renderer.blockRenderInfo.mvp = viewProj * modelMatrix;
                m_renderer.blockRenderInfo.cameraOffset = coordinatesVec;
                m_renderer.drawBlocks(mesh.waterMesh);
            }
            doRenderThreadJobs();
        }
    }

    m_renderingFrame = false;
}

void ClientWorld::doRenderThreadJobs() {
    for (int8_t threadNum = 0; threadNum < m_numChunkLoadingThreads; threadNum++) {
        if (m_chunkMeshReady[threadNum]) {
            uploadChunkMesh(threadNum);
            // lock release 
            std::lock_guard<std::mutex> lock(m_chunkMeshReadyMtx[threadNum]);
            chunkMeshUploaded[threadNum] = true;
            m_chunkMeshReady[threadNum] = false;
            // notify consumer when done 
            m_chunkMeshReadyCV[threadNum].notify_one();
        }
    }
}

void ClientWorld::updateMeshes() {
    m_renderThreadWaitingForMeshUpdatesMtx.lock();
    m_renderThreadWaitingForMeshUpdates = true;
    std::lock_guard<std::mutex> meshUpdatesLock(m_meshUpdatesMtx);
    m_renderThreadWaitingForMeshUpdates = false;
    m_renderThreadWaitingForMeshUpdatesMtx.unlock();
    std::lock_guard<std::mutex> unmeshedChunksLock(m_unmeshedChunksMtx);

    auto it = m_meshesToUpdate.begin();
    while (it != m_meshesToUpdate.end()) {
        auto meshIt = m_meshes.find(*it);
        if (meshIt == m_meshes.end())
        {
            m_unmeshedChunks.insert(*it);
            m_meshUpdates.insert(*it);
            m_recentChunksBuilt.push(*it);
            it = m_meshesToUpdate.erase(it);
            continue;
        }

        m_meshesToUnload[
            (m_renderer.getVulkanEngine().getFrameDataIndex()
            + VulkanEngine::MAX_FRAMES_IN_FLIGHT - 1) % VulkanEngine::MAX_FRAMES_IN_FLIGHT
        ].push_back(meshIt->second);
        m_unmeshedChunks.insert(*it);
        m_meshes.erase(meshIt);
        m_meshUpdates.insert(*it);
        m_recentChunksBuilt.push(*it);
        it = m_meshesToUpdate.erase(it);
    }
}

void ClientWorld::updatePlayerPos(IVec3 playerBlockCoords, Vec3 playerSubBlockCoords) {
    IVec3 newPlayerChunkPosition = Chunk::getChunkCoords(playerBlockCoords);
    m_newPlayerChunkPosition[0] = newPlayerChunkPosition.x;
    m_newPlayerChunkPosition[1] = newPlayerChunkPosition.y;
    m_newPlayerChunkPosition[2] = newPlayerChunkPosition.z;

    unmeshCompleted = (m_playerChunkPosition[0] == m_newPlayerChunkPosition[0])
        && (m_playerChunkPosition[1] == m_newPlayerChunkPosition[1])
        && (m_playerChunkPosition[2] == m_newPlayerChunkPosition[2]);
    m_unmeshNeeded = !unmeshCompleted;

    for (int8_t i = 0; i < 3; i++) {
        m_updatingPlayerChunkPosition[i] = m_newPlayerChunkPosition[i];
    }

    bool readyToRelable = false;
    while (m_unmeshNeeded && (!readyToRelable)) {
        doRenderThreadJobs();
        //wait for all the mesh builder threads to finish their jobs
        for (int8_t threadNum = 0; threadNum < m_numChunkLoadingThreads; threadNum++) {
            readyToRelable |= !m_threadWaiting[threadNum];
        }
        readyToRelable = !readyToRelable;
    }

    integratedServer.updatePlayerPos(0, playerBlockCoords, playerSubBlockCoords, m_unmeshNeeded);

    if (m_unmeshNeeded) {
        unmeshChunks();
        unmeshCompleted = true;
        m_unmeshNeeded = false;
        m_chunkRequestScheduled = true;
        // lock release 
        std::lock_guard<std::mutex> lock(m_unmeshNeededMtx);
        // notify consumer when done
        m_unmeshNeededCV.notify_all();
    }
}

void ClientWorld::loadChunksAroundPlayerSingleplayer(int8_t threadNum) {
    while (m_unmeshNeeded && (m_meshUpdates.size() == 0)) {
        m_threadWaiting[threadNum] = true;
        // locking
        std::unique_lock<std::mutex> lock(m_unmeshNeededMtx);
        // waiting
        m_unmeshNeededCV.wait(lock, [] { return unmeshCompleted; });
        m_threadWaiting[threadNum] = false;
    }
    if (m_meshUpdates.empty()) {
        IVec3 chunkPosition;
        if (integratedServer.loadNextChunk(&chunkPosition)) {
            m_unmeshedChunksMtx.lock();
            m_unmeshedChunks.insert(chunkPosition);
            m_recentChunksBuilt.push(chunkPosition);
            m_unmeshedChunksMtx.unlock();
        }
    }
    buildMeshesForNewChunksWithNeighbours(threadNum);
}

void ClientWorld::loadChunksAroundPlayerMultiplayer(int8_t threadNum) {
    while (m_unmeshNeeded && (m_meshUpdates.size() == 0)) {
        m_threadWaiting[threadNum] = true;
        // locking
        std::unique_lock<std::mutex> lock(m_unmeshNeededMtx);
        // waiting
        m_unmeshNeededCV.wait(lock, [] { return unmeshCompleted; });
        m_threadWaiting[threadNum] = false;
    }
    buildMeshesForNewChunksWithNeighbours(threadNum);
}

void ClientWorld::loadChunkFromPacket(Packet<uint8_t, 9 * constants::CHUNK_SIZE *
    constants::CHUNK_SIZE * constants::CHUNK_SIZE>& payload) {
    IVec3 chunkPosition;
    integratedServer.loadChunkFromPacket(payload, chunkPosition);
    std::lock_guard<std::mutex> lock(m_unmeshedChunksMtx);
    m_unmeshedChunks.insert(chunkPosition);
    m_recentChunksBuilt.push(chunkPosition);
}

void ClientWorld::unmeshChunks() {
    m_updatingPlayerChunkPosition[0] = m_newPlayerChunkPosition[0];
    m_updatingPlayerChunkPosition[1] = m_newPlayerChunkPosition[1];
    m_updatingPlayerChunkPosition[2] = m_newPlayerChunkPosition[2];
    //unload any meshes and chunks that are out of render distance
    m_unmeshedChunksMtx.lock();
    for (auto it = m_meshes.begin(); it != m_meshes.end();) {
        const IVec3 chunkPosition = it->first;
        float distance = (chunkPosition.x - m_updatingPlayerChunkPosition[0])
            * (chunkPosition.x - m_updatingPlayerChunkPosition[0]);
        distance += (chunkPosition.y - m_updatingPlayerChunkPosition[1])
            * (chunkPosition.y - m_updatingPlayerChunkPosition[1]);
        distance += (chunkPosition.z - m_updatingPlayerChunkPosition[2])
            * (chunkPosition.z - m_updatingPlayerChunkPosition[2]);
        if (distance > ((m_renderDistance + 0.999f) * (m_renderDistance + 0.999f)))
        {
            m_meshesToUnload[
                (m_renderer.getVulkanEngine().getFrameDataIndex()
                + VulkanEngine::MAX_FRAMES_IN_FLIGHT - 1) % VulkanEngine::MAX_FRAMES_IN_FLIGHT
            ].push_back(it->second);
            m_unmeshedChunks.insert(it->first);
            it = m_meshes.erase(it);
        }
        else
            ++it;
    }
    // Remove any chunks from unmeshedChunks that have just been unloaded
    for (auto it = m_unmeshedChunks.begin(); it != m_unmeshedChunks.end(); ) {
        if (!integratedServer.chunkManager.chunkLoaded(*it)) {
            it = m_unmeshedChunks.erase(it);
        } else {
            ++it;
        }
    }
    m_unmeshedChunksMtx.unlock();

    //update the player's chunk position
    m_playerChunkPosition[0] = m_updatingPlayerChunkPosition[0];
    m_playerChunkPosition[1] = m_updatingPlayerChunkPosition[1];
    m_playerChunkPosition[2] = m_updatingPlayerChunkPosition[2];
}

bool ClientWorld::chunkHasNeighbours(const IVec3& chunkPosition) {
    for (uint8_t i = 0; i < 27; i++) {
        if (!(integratedServer.chunkManager.chunkLoaded(chunkPosition + m_neighbouringChunkIncludingDiaganalOffsets[i]))) {
            return false;
        }
    }
    return true;
}

void ClientWorld::addChunksToRemesh(std::vector<IVec3>& chunksToRemesh, const IVec3&
    modifiedBlockPos, const IVec3& modifiedBlockChunk) {
    chunksToRemesh.push_back(modifiedBlockChunk);
    IVec3 blockPosInChunk = modifiedBlockPos - modifiedBlockChunk * constants::CHUNK_SIZE;
    if (blockPosInChunk.x == 0) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(-1, 0, 0));
    }
    else if (blockPosInChunk.x == constants::CHUNK_SIZE - 1) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(1, 0, 0));
    }
    if (blockPosInChunk.y == 0) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(0, -1, 0));
    }
    else if (blockPosInChunk.y == constants::CHUNK_SIZE - 1) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(0, 1, 0));
    }
    if (blockPosInChunk.z == 0) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(0, 0, -1));
    }
    else if (blockPosInChunk.z == constants::CHUNK_SIZE - 1) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(0, 0, 1));
    }
}

void ClientWorld::unloadMesh(MeshData& mesh)
{
    if (mesh.blockMesh.indexCount > 0)
    {
        m_renderer.getVulkanEngine().destroyBuffer(mesh.blockMesh.vertexBuffer);
        m_renderer.getVulkanEngine().destroyBuffer(mesh.blockMesh.indexBuffer);
    }

    if (mesh.waterMesh.indexCount > 0)
    {
        m_renderer.getVulkanEngine().destroyBuffer(mesh.waterMesh.vertexBuffer);
        m_renderer.getVulkanEngine().destroyBuffer(mesh.waterMesh.indexBuffer);
    }
}

void ClientWorld::addChunkMesh(const IVec3& chunkPosition, int8_t threadNum) {
    int chunkCoords[3] = { chunkPosition.x, chunkPosition.y, chunkPosition.z };

    //generate the mesh
    MeshBuilder(
        integratedServer.chunkManager.getChunk(chunkPosition),
        integratedServer,
        m_chunkVertices[threadNum],
        &m_numChunkVertices[threadNum],
        m_chunkIndices[threadNum],
        &m_numChunkIndices[threadNum],
        m_chunkWaterVertices[threadNum],
        &m_numChunkWaterVertices[threadNum],
        m_chunkWaterIndices[threadNum],
        &m_numChunkWaterIndices[threadNum]
    ).buildMesh();

    //if the mesh is empty dont upload it to save interrupting the render thread
    if ((m_numChunkIndices[threadNum] == 0) && (m_numChunkWaterIndices[threadNum] == 0)) {
        m_meshUpdatesMtx.lock();
        while (m_renderThreadWaitingForMeshUpdates) {
            m_meshUpdatesMtx.unlock();
            m_renderThreadWaitingForMeshUpdatesMtx.lock();
            m_meshUpdatesMtx.lock();
            m_renderThreadWaitingForMeshUpdatesMtx.unlock();
        }
        auto it = m_meshUpdates.find(chunkPosition);
        if (it != m_meshUpdates.end()) {
            m_meshUpdates.erase(it);
        }
        m_meshUpdatesMtx.unlock();
        return;
    }

    //wait for the render thread to upload the mesh to the GPU
    m_chunkPosition[threadNum] = chunkPosition;
    m_chunkMeshReady[threadNum] = true;
    chunkMeshUploaded[threadNum] = false;

    if (!m_meshUpdates.contains(chunkPosition)) {
    m_meshedChunksDistance = (chunkPosition.x - m_playerChunkPosition[0]) * (chunkPosition.x - m_playerChunkPosition[0])
        + (chunkPosition.y - m_playerChunkPosition[1]) * (chunkPosition.y - m_playerChunkPosition[1])
        + (chunkPosition.z - m_playerChunkPosition[2]) * (chunkPosition.z - m_playerChunkPosition[2]);
    }

    // locking
    std::unique_lock<std::mutex> lock(m_chunkMeshReadyMtx[threadNum]);
    // waiting
    while (!chunkMeshUploaded[threadNum]) {
        m_chunkMeshReadyCV[threadNum].wait(lock);
    }
}

void ClientWorld::uploadChunkMesh(int8_t threadNum) {
    MeshData newMesh;

    if (m_numChunkIndices[threadNum] > 0)
    {
        newMesh.blockMesh = m_renderer.getVulkanEngine().uploadMesh(
            std::span{ m_chunkVertices[threadNum], m_numChunkVertices[threadNum] },
            std::span{ m_chunkIndices[threadNum], m_numChunkIndices[threadNum] }
        );
    }
    else
    {
        newMesh.blockMesh.indexCount = 0;
    }

    if (m_numChunkWaterVertices[threadNum] > 0)
    {
        newMesh.waterMesh = m_renderer.getVulkanEngine().uploadMesh(
            std::span{ m_chunkWaterVertices[threadNum], m_numChunkWaterVertices[threadNum] },
            std::span{ m_chunkWaterIndices[threadNum], m_numChunkWaterIndices[threadNum] }
        );
    }
    else
    {
        newMesh.waterMesh.indexCount = 0;
    }

    m_renderThreadWaitingForMeshUpdatesMtx.lock();
    m_renderThreadWaitingForMeshUpdates = true;
    std::lock_guard<std::mutex> lock(m_meshUpdatesMtx);
    m_renderThreadWaitingForMeshUpdates = false;
    m_renderThreadWaitingForMeshUpdatesMtx.unlock();
    m_meshes[m_chunkPosition[threadNum]] = newMesh;
    auto it = m_meshUpdates.find(m_chunkPosition[threadNum]);
    if (it != m_meshUpdates.end()) {
        m_meshUpdates.erase(it);
    }
}

void ClientWorld::buildMeshesForNewChunksWithNeighbours(int8_t threadNum) {
    if (m_recentChunksBuilt.size() > 0) {
        m_unmeshedChunksMtx.lock();
        if (m_recentChunksBuilt.size() > 0) {
            IVec3 newChunkPosition = m_recentChunksBuilt.front();
            m_recentChunksBuilt.pop();
            for (int i = 0; i < 27; i++) {
                IVec3 chunkPosition = newChunkPosition + m_neighbouringChunkIncludingDiaganalOffsets[i];
                if (m_unmeshedChunks.contains(chunkPosition) && chunkHasNeighbours(chunkPosition)) {
                    m_unmeshedChunks.erase(chunkPosition);
                    m_unmeshedChunksMtx.unlock();
                    Chunk& chunk = integratedServer.chunkManager.getChunk(chunkPosition);
                    if (!chunk.isSkyLightUpToDate()) {
                        Chunk::s_checkingNeighbourSkyRelightsMtx.lock();
                        bool neighbourBeingRelit = true;
                        while (neighbourBeingRelit) {
                            neighbourBeingRelit = false;
                            for (uint32_t i = 0; i < 6; i++) {
                                neighbourBeingRelit |= integratedServer.chunkManager.getChunk(chunkPosition + s_neighbouringChunkOffsets[i]).isSkyLightBeingRelit();
                            }
                            if (neighbourBeingRelit) {
                                Chunk::s_checkingNeighbourSkyRelightsMtx.unlock();
                                std::this_thread::sleep_for(std::chrono::microseconds(100));
                                Chunk::s_checkingNeighbourSkyRelightsMtx.lock();
                            }
                        }
                        Chunk::s_checkingNeighbourSkyRelightsMtx.unlock();
                        chunk.setSkyLightBeingRelit(true);
                        chunk.clearSkyLight();
                        bool neighbouringChunksToRelight[6];
                        bool chunksToRemesh[7];
                        Lighting::propagateSkyLight(
                            chunkPosition, integratedServer.chunkManager.getWorldChunks(),
                            neighbouringChunksToRelight, chunksToRemesh,
                            integratedServer.getResourcePack()
                        );
                        chunk.setSkyLightToBeUpToDate();
                    }
                    addChunkMesh(chunkPosition, threadNum);
                    m_unmeshedChunksMtx.lock();
                }
            }
        }
        m_unmeshedChunksMtx.unlock();
    }
    // else if (!m_singleplayer && m_clientID > -1)
    // {
    //     requestMoreChunks();
    // }
}

uint8_t ClientWorld::shootRay(glm::vec3 startSubBlockPos, int* startBlockPosition, glm::vec3 direction, int* breakBlockCoords, int* placeBlockCoords) {
    //TODO: improve this to make it need less steps
    glm::vec3 rayPos = startSubBlockPos;
    int blockPos[3];
    int steps = 0;
    while (steps < 180) {
        rayPos += direction * 0.025f;
        for (uint8_t ii = 0; ii < 3; ii++) {
            blockPos[ii] = floor(rayPos[ii]) + startBlockPosition[ii];
        }
        uint8_t blockType = integratedServer.chunkManager.getBlock(blockPos);
        if ((blockType != 0) && (blockType != 4)) {
            bool hit = true;
            for (uint8_t ii = 0; ii < 3; ii++) {
                if (rayPos[ii] < blockPos[ii] - startBlockPosition[ii] + integratedServer.getResourcePack().getBlockData(blockType).model
                    ->boundingBoxVertices[ii] + 0.5f || rayPos[ii] > blockPos[ii] - startBlockPosition[ii] + integratedServer.getResourcePack().
                    getBlockData(blockType).model->boundingBoxVertices[ii + 15] + 0.5f) {
                    hit = false;
                }
            }
            if (hit) {
                for (uint8_t ii = 0; ii < 3; ii++) {
                    breakBlockCoords[ii] = blockPos[ii];
                }

                bool equal = true;
                while (equal) {
                    rayPos -= direction * 0.025f;
                    for (uint8_t ii = 0; ii < 3; ii++) {
                        placeBlockCoords[ii] = floor(rayPos[ii]) + startBlockPosition[ii];
                        equal &= placeBlockCoords[ii] == blockPos[ii];
                    }
                }

                for (uint8_t ii = 0; ii < 3; ii++) {
                    placeBlockCoords[ii] = floor(rayPos[ii]) + startBlockPosition[ii];
                }
                return blockType;
            }
        }
        steps++;
    }
    return 0;
}

void ClientWorld::replaceBlock(const IVec3& blockCoords, uint8_t blockType) {
    IVec3 chunkPosition = Chunk::getChunkCoords(blockCoords);

    // LOG(std::to_string((uint32_t)m_integratedServer.getBlockLight(blockCoords)) + " block light level\n");
    uint8_t originalBlockType = integratedServer.chunkManager.getBlock(blockCoords);
    integratedServer.chunkManager.setBlock(blockCoords, blockType);

    std::vector<IVec3> chunksToRemesh;
    addChunksToRemesh(chunksToRemesh, blockCoords, chunkPosition);

    auto tp1 = std::chrono::high_resolution_clock::now();
    Lighting::relightChunksAroundBlock(blockCoords, chunkPosition, originalBlockType, blockType,
        chunksToRemesh, integratedServer.chunkManager.getWorldChunks(), integratedServer.getResourcePack());
    auto tp2 = std::chrono::high_resolution_clock::now();
    // LOG(std::to_string(chunksToRemesh.size()) + " chunks remeshed\n");
    // LOG("relight took " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count()) + "us");

    for (auto& chunk : chunksToRemesh)
    {
        if (chunkHasNeighbours(chunk))
            m_meshesToUpdate.insert(chunk);
    }
}

void ClientWorld::setThreadWaiting(uint8_t threadNum, bool value) {
    while (m_unmeshNeeded && (m_meshUpdates.size() == 0)) {
        m_threadWaiting[threadNum] = true;
        // locking
        std::unique_lock<std::mutex> lock(m_unmeshNeededMtx);
        // waiting
        m_unmeshNeededCV.wait(lock, [] { return unmeshCompleted; });
        m_threadWaiting[threadNum] = false;
    }
    m_threadWaiting[threadNum] = value;
}

void ClientWorld::requestMoreChunks()
{
    if (integratedServer.updateClientChunkLoadingTarget() || m_chunkRequestScheduled)
    {
        ServerPlayer& player = integratedServer.getPlayer(0);
        Packet<int64_t, 3> payload(m_clientID, PacketType::ChunkRequest, 3);
        payload[0] = player.incrementNumChunkRequests();
        payload[1] = player.getChunkLoadingTarget();
        payload[2] = player.getTargetBufferSize();
        m_networkingMtx.lock();
        ENetPacket* packet = enet_packet_create(
            (const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_RELIABLE
        );
        enet_peer_send(m_peer, 0, packet);
        m_networkingMtx.unlock();
        m_chunkRequestScheduled = false;
    }
}

void ClientWorld::unloadMeshes()
{
    std::lock_guard<std::mutex> lock(m_unmeshedChunksMtx);
    for (auto& mesh : m_meshesToUnload[m_renderer.getVulkanEngine().getFrameDataIndex()])
        unloadMesh(mesh);

    m_meshesToUnload[m_renderer.getVulkanEngine().getFrameDataIndex()].clear();
}

void ClientWorld::unloadAllMeshes()
{
    while (!m_meshes.empty())
    {
        unloadMesh(m_meshes.begin()->second);
        m_unmeshedChunks.insert(m_meshes.begin()->first);
        m_meshes.erase(m_meshes.begin());
    }

    for (auto meshesToUnload : m_meshesToUnload)
    {
        for (auto& mesh : meshesToUnload)
            unloadMesh(mesh);

        meshesToUnload.clear();
    }
}

void ClientWorld::freeEntityMeshes()
{
    for (GPUDynamicMeshBuffers mesh : m_entityMeshes)
    {
        m_renderer.getVulkanEngine().destroyHostVisibleAndDeviceLocalBuffer(mesh.vertexBuffer);
        m_renderer.getVulkanEngine().destroyHostVisibleAndDeviceLocalBuffer(mesh.indexBuffer);
    }
}

}  // namespace lonelycube::client
