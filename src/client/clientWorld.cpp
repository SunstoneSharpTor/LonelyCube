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

ClientWorld::ClientWorld(
    int renderDistance, uint64_t seed, bool singleplayer, const IVec3& playerPos,
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

    // Mesh-building data
    m_chunkVertices.resize(m_numChunkLoadingThreads);
    m_chunkIndices.resize(m_numChunkLoadingThreads);
    m_chunkWaterVertices.resize(m_numChunkLoadingThreads);
    m_chunkWaterIndices.resize(m_numChunkLoadingThreads);
    m_chunkPosition.resize(m_numChunkLoadingThreads);
    m_chunkMeshReady.resize(m_numChunkLoadingThreads);
    m_chunkMeshReadyMtx = std::make_unique<std::mutex[]>(m_numChunkLoadingThreads);
    m_chunkMeshReadyCV = std::make_unique<std::condition_variable[]>(m_numChunkLoadingThreads);
    m_threadWaiting.resize(m_numChunkLoadingThreads);
    m_unmeshNeeded = false;
    m_readyForChunkUnload = false;

    m_entityMeshes.reserve(VulkanEngine::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < VulkanEngine::MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_entityMeshes.push_back(m_renderer.getVulkanEngine().allocateDynamicMesh(
            1680000 * sizeof(float), 360000 * sizeof(uint32_t)
        ));
    }

    for (int i = 0; i < m_numChunkLoadingThreads; i++) {
        m_chunkVertices[i].reserve(
            12 * 6 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE
        );
        m_chunkIndices[i].reserve(
            18 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE
        );
        m_chunkWaterVertices[i].reserve(
            12 * 6 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE
        );
        m_chunkWaterIndices[i].reserve(
            18 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE
        );
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

    m_renderer.blockRenderInfo.mvp = viewProj;
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
    int prevChunkDistance = -1;
    for (std::size_t i = 0; i < m_meshes.size(); i++) {
        IVec3 chunkCoordinates = m_meshes[i].chunkPosition * constants::CHUNK_SIZE - playerBlockPos;
        if (m_meshes[i].blockMesh.indexCount > 0) {
            glm::vec3 coordinatesVec(chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
            AABB aabb(coordinatesVec, coordinatesVec + glm::vec3(constants::CHUNK_SIZE));
            if (aabb.isOnFrustum(viewFrustum)) {
                m_renderer.blockRenderInfo.chunkCoordinates = coordinatesVec;
                m_renderer.blockRenderInfo.playerSubBlockPos = playerSubBlockPos;
                m_renderer.drawBlocks(m_meshes[i].blockMesh);
            }

            doRenderThreadJobs();
        }

        // Sort the chunks depending on distance to the camera
        chunkCoordinates += IVec3(constants::CHUNK_SIZE / 2);
        const int chunkDistance = chunkCoordinates.x * chunkCoordinates.x
            + chunkCoordinates.y * chunkCoordinates.y + chunkCoordinates.z * chunkCoordinates.z;
        if (chunkDistance < prevChunkDistance)
        {
            const MeshData temp = m_meshes[i - 1];
            m_meshes[i - 1] = m_meshes[i];
            m_meshes[i] = temp;
            if (m_meshArrayIndices.contains(m_meshes[i - 1].chunkPosition))
                m_meshArrayIndices.at(m_meshes[i - 1].chunkPosition) = i - 1;
            if (m_meshArrayIndices.contains(m_meshes[i].chunkPosition))
                m_meshArrayIndices.at(m_meshes[i].chunkPosition) = i;
        }
        prevChunkDistance = chunkDistance;
    }

    while (
        !m_meshes.empty()
        && m_meshes.back().blockMesh.indexCount == 0
        && m_meshes.back().waterMesh.indexCount == 0
    ) {
        m_meshes.pop_back();
    }

    // Render entities
    GPUDynamicMeshBuffers& entityMesh = m_entityMeshes[
        m_renderer.getVulkanEngine().getFrameDataIndex()
    ];
    m_renderer.blockRenderInfo.chunkCoordinates = glm::vec3(0.0f);
    m_renderer.blockRenderInfo.playerSubBlockPos = -playerSubBlockPos;
    m_renderer.drawEntities(entityMesh);

    // Render water
    m_renderer.beginDrawingWater();
    for (const auto& mesh : m_meshes) {
        if (mesh.waterMesh.indexCount > 0) {
            IVec3 chunkCoordinates = mesh.chunkPosition * constants::CHUNK_SIZE - playerBlockPos;
            glm::vec3 coordinatesVec(chunkCoordinates.x, chunkCoordinates.y, chunkCoordinates.z);
            AABB aabb(coordinatesVec, coordinatesVec + glm::vec3(constants::CHUNK_SIZE));
            if (aabb.isOnFrustum(viewFrustum)) {
                m_renderer.blockRenderInfo.chunkCoordinates = coordinatesVec;
                m_renderer.blockRenderInfo.playerSubBlockPos = playerSubBlockPos;
                m_renderer.drawBlocks(mesh.waterMesh);
            }
            doRenderThreadJobs();
        }
    }

    // LOG(std::to_string(m_meshes.size()) + ", " + std::to_string(m_meshArrayIndices.size()));

    m_renderingFrame = false;
}

void ClientWorld::doRenderThreadJobs()
{
    for (int threadNum = 0; threadNum < m_numChunkLoadingThreads; threadNum++) {
        if (m_chunkMeshReady[threadNum]) {
            uploadChunkMesh(threadNum);
            std::unique_lock<std::mutex> lock(m_chunkMeshReadyMtx[threadNum]);
            m_chunkMeshReady[threadNum] = false;
            lock.unlock();
            m_chunkMeshReadyCV[threadNum].notify_all();
        }
    }
}

void ClientWorld::updateMeshes()
{
    m_renderThreadWaitingForMeshUpdatesMtx.lock();
    m_renderThreadWaitingForMeshUpdates = true;
    std::lock_guard<std::mutex> meshUpdatesLock(m_meshUpdatesMtx);
    m_renderThreadWaitingForMeshUpdates = false;
    m_renderThreadWaitingForMeshUpdatesMtx.unlock();
    std::lock_guard<std::mutex> unmeshedChunksLock(m_unmeshedChunksMtx);

    std::lock_guard<std::mutex> lock(m_meshesToUpdateMtx);
    auto it = m_meshesToUpdate.begin();
    while (it != m_meshesToUpdate.end()) {
        m_unmeshedChunks.insert(*it);
        m_meshUpdates.insert(*it);
        m_recentChunksBuilt.push_front(*it);
        it = m_meshesToUpdate.erase(it);
    }
}

void ClientWorld::updatePlayerPos(IVec3 playerBlockCoords, Vec3 playerSubBlockCoords)
{
    IVec3 newPlayerChunkPosition = Chunk::getChunkCoords(playerBlockCoords);
    m_newPlayerChunkPosition[0] = newPlayerChunkPosition.x;
    m_newPlayerChunkPosition[1] = newPlayerChunkPosition.y;
    m_newPlayerChunkPosition[2] = newPlayerChunkPosition.z;

    m_unmeshNeeded = !((m_playerChunkPosition[0] == m_newPlayerChunkPosition[0])
        && (m_playerChunkPosition[1] == m_newPlayerChunkPosition[1])
        && (m_playerChunkPosition[2] == m_newPlayerChunkPosition[2]));

    for (int i = 0; i < 3; i++) {
        m_updatingPlayerChunkPosition[i] = m_newPlayerChunkPosition[i];
    }

    integratedServer.updatePlayerPos(0, playerBlockCoords, playerSubBlockCoords, m_unmeshNeeded);

    if (m_unmeshNeeded)
    {
        std::lock_guard<std::mutex> lock(m_readyForChunkUnloadMtx);
        m_readyForChunkUnload = true;
        m_readyForChunkUnloadCV.notify_one();
    }
}

void ClientWorld::unloadOutOfRangeMeshesIfNeeded()
{
    m_readyForChunkUnloadMtx.lock();
    if (m_unmeshNeeded && !(m_readyForChunkUnload || m_unloadingChunks))
    {
        m_readyForChunkUnloadMtx.unlock();
        auto tp1 = std::chrono::high_resolution_clock::now();
        unmeshChunks();
        std::unique_lock<std::mutex> lock(m_unmeshNeededMtx);
        m_unmeshNeeded = false;
        m_chunkRequestScheduled = true;
        lock.unlock();
        m_unmeshNeededCV.notify_all();
        m_readyForChunkUnloadCV.notify_one();
        auto tp2 = std::chrono::high_resolution_clock::now();
        LOG("waited " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count()) + "us for mesh unloads to be queued");
    }
    else
        m_readyForChunkUnloadMtx.unlock();
}

void ClientWorld::waitIfMeshesNeedUnloading(int threadNum)
{
    while (m_unmeshNeeded && (m_meshUpdates.size() == 0))
    {
        m_threadWaiting[threadNum] = true;
        if (threadNum == 0)
        {
            // Wait to unload chunks
            std::unique_lock<std::mutex> lock(m_readyForChunkUnloadMtx);
            while (!m_readyForChunkUnload)
                m_readyForChunkUnloadCV.wait(lock);

            m_unloadingChunks = true;
            m_readyForChunkUnload = false;
            lock.unlock();

            // Wait for all the chunk loader threads to finish their jobs
            bool waitingForChunkLoaderThreads = true;
            while (m_unmeshNeeded && waitingForChunkLoaderThreads) {
                waitingForChunkLoaderThreads = false;
                for (int threadNum = 0; threadNum < m_numChunkLoadingThreads; threadNum++) {
                    waitingForChunkLoaderThreads |= !m_threadWaiting[threadNum];
                }
            }

            integratedServer.unloadChunksOutOfRange();

            // Wait for chunks to be unmeshed
            lock.lock();
            m_unloadingChunks = false;
            while (m_unmeshNeeded && !m_readyForChunkUnload)
                m_readyForChunkUnloadCV.wait(lock);
        }
        else
        {
            // Wait for chunks to be unmeshed
            std::unique_lock<std::mutex> lock(m_unmeshNeededMtx);
            while(m_unmeshNeeded)
                m_unmeshNeededCV.wait(lock);
            m_threadWaiting[threadNum] = false;
        }
    }
}

void ClientWorld::loadChunksAroundPlayerSingleplayer(int threadNum)
{
    waitIfMeshesNeedUnloading(threadNum);
    if (m_meshUpdates.empty())
    {
        IVec3 chunkPosition;
        if (integratedServer.loadNextChunk(&chunkPosition)) {
            m_unmeshedChunksMtx.lock();
            m_unmeshedChunks.insert(chunkPosition);
            m_recentChunksBuilt.push_back(chunkPosition);
            m_unmeshedChunksMtx.unlock();
        }
    }
    buildMeshesForNewChunksWithNeighbours(threadNum);
}

bool ClientWorld::loadChunksAroundPlayerMultiplayer(int threadNum)
{
    waitIfMeshesNeedUnloading(threadNum);
    return buildMeshesForNewChunksWithNeighbours(threadNum);
}

void ClientWorld::loadChunkFromPacket(
    Packet<uint8_t, 9 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE>&
    payload
) {
    IVec3 chunkPosition;
    integratedServer.loadChunkFromPacket(payload, chunkPosition);
    std::lock_guard<std::mutex> lock(m_unmeshedChunksMtx);
    m_unmeshedChunks.insert(chunkPosition);
    m_recentChunksBuilt.push_back(chunkPosition);
}

void ClientWorld::unmeshChunks()
{
    m_updatingPlayerChunkPosition[0] = m_newPlayerChunkPosition[0];
    m_updatingPlayerChunkPosition[1] = m_newPlayerChunkPosition[1];
    m_updatingPlayerChunkPosition[2] = m_newPlayerChunkPosition[2];
    //unload any meshes and chunks that are out of render distance
    m_unmeshedChunksMtx.lock();
    for (std::size_t i = 0; i < m_meshes.size(); i++)
    {
        if (m_meshes[i].blockMesh.indexCount == 0 && m_meshes[i].waterMesh.indexCount == 0)
            continue;

        const IVec3 chunkPosition = m_meshes[i].chunkPosition;
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
            ].push_back(m_meshes[i]);
            m_meshes[i].blockMesh.indexCount = 0;
            m_meshes[i].waterMesh.indexCount = 0;
            m_meshArrayIndices.erase(m_meshes[i].chunkPosition);
            m_unmeshedChunks.insert(m_meshes[i].chunkPosition);
        }
    }
    // Remove any chunks from unmeshedChunks that have just been unloaded
    for (auto it = m_unmeshedChunks.begin(); it != m_unmeshedChunks.end(); )
    {
        if (!integratedServer.chunkManager.chunkLoaded(*it))
            it = m_unmeshedChunks.erase(it);
        else
            ++it;
    }
    m_unmeshedChunksMtx.unlock();

    //update the player's chunk position
    m_playerChunkPosition[0] = m_updatingPlayerChunkPosition[0];
    m_playerChunkPosition[1] = m_updatingPlayerChunkPosition[1];
    m_playerChunkPosition[2] = m_updatingPlayerChunkPosition[2];
}

bool ClientWorld::chunkHasNeighbours(const IVec3& chunkPosition) {
    for (int i = 0; i < 27; i++)
    {
        if (!(integratedServer.chunkManager.chunkLoaded(chunkPosition + m_neighbouringChunkIncludingDiaganalOffsets[i])))
            return false;
    }
    return true;
}

void ClientWorld::addChunksToRemesh(std::vector<IVec3>& chunksToRemesh, const IVec3&
    modifiedBlockPos, const IVec3& modifiedBlockChunk)
{
    chunksToRemesh.push_back(modifiedBlockChunk);
    IVec3 blockPosInChunk = modifiedBlockPos - modifiedBlockChunk * constants::CHUNK_SIZE;
    if (blockPosInChunk.x == 0)
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(-1, 0, 0));
    else if (blockPosInChunk.x == constants::CHUNK_SIZE - 1)
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(1, 0, 0));

    if (blockPosInChunk.y == 0)
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(0, -1, 0));
    else if (blockPosInChunk.y == constants::CHUNK_SIZE - 1)
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(0, 1, 0));

    if (blockPosInChunk.z == 0)
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(0, 0, -1));
    else if (blockPosInChunk.z == constants::CHUNK_SIZE - 1)
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(0, 0, 1));
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

void ClientWorld::addChunkMesh(const IVec3& chunkPosition, int threadNum) {
    int chunkCoords[3] = { chunkPosition.x, chunkPosition.y, chunkPosition.z };

    //generate the mesh
    MeshBuilder(
        integratedServer.chunkManager.getChunk(chunkPosition), integratedServer,
        m_chunkVertices[threadNum], m_chunkIndices[threadNum], m_chunkWaterVertices[threadNum],
        m_chunkWaterIndices[threadNum]
    ).buildMesh();

    //if the mesh is empty dont upload it to save interrupting the render thread
    if ((m_chunkIndices[threadNum].size() == 0) && (m_chunkWaterIndices[threadNum].size() == 0))
    {
        m_meshUpdatesMtx.lock();
        while (m_renderThreadWaitingForMeshUpdates)
        {
            m_meshUpdatesMtx.unlock();
            m_renderThreadWaitingForMeshUpdatesMtx.lock();
            m_meshUpdatesMtx.lock();
            m_renderThreadWaitingForMeshUpdatesMtx.unlock();
        }
        auto it = m_meshUpdates.find(chunkPosition);
        if (it != m_meshUpdates.end())
            m_meshUpdates.erase(it);
        m_meshUpdatesMtx.unlock();
        return;
    }

    //wait for the render thread to upload the mesh to the GPU
    m_chunkPosition[threadNum] = chunkPosition;
    std::unique_lock<std::mutex> lock(m_chunkMeshReadyMtx[threadNum]);
    m_chunkMeshReady[threadNum] = true;

    if (!m_meshArrayIndices.contains(chunkPosition))
    {
        m_meshedChunksDistance = (chunkPosition.x - m_playerChunkPosition[0]) *
        (chunkPosition.x - m_playerChunkPosition[0]) +
        (chunkPosition.y - m_playerChunkPosition[1]) *
        (chunkPosition.y - m_playerChunkPosition[1]) +
        (chunkPosition.z - m_playerChunkPosition[2]) *
        (chunkPosition.z - m_playerChunkPosition[2]);
    }

    while (m_chunkMeshReady[threadNum])
        m_chunkMeshReadyCV[threadNum].wait(lock);
}

void ClientWorld::uploadChunkMesh(int threadNum) {
    MeshData newMesh;
    newMesh.chunkPosition = m_chunkPosition[threadNum];

    if (m_chunkIndices[threadNum].size() > 0)
    {
        newMesh.blockMesh = m_renderer.getVulkanEngine().uploadMesh(
            m_chunkVertices[threadNum], m_chunkIndices[threadNum]
        );
    }
    else
    {
        newMesh.blockMesh.indexCount = 0;
    }

    if (m_chunkWaterVertices[threadNum].size() > 0)
    {
        newMesh.waterMesh = m_renderer.getVulkanEngine().uploadMesh(
            m_chunkWaterVertices[threadNum], m_chunkWaterIndices[threadNum]
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

    m_meshUpdates.erase(m_chunkPosition[threadNum]);
    auto meshArrayIndicesItr = m_meshArrayIndices.find(m_chunkPosition[threadNum]);
    if (meshArrayIndicesItr != m_meshArrayIndices.end())
    {
        m_meshesToUnload[
            (m_renderer.getVulkanEngine().getFrameDataIndex() +
            VulkanEngine::MAX_FRAMES_IN_FLIGHT - !m_renderingFrame) %
            VulkanEngine::MAX_FRAMES_IN_FLIGHT
        ].push_back(m_meshes[meshArrayIndicesItr->second]);
        m_meshes[meshArrayIndicesItr->second] = newMesh;
        return;
    }

    m_meshArrayIndices[m_chunkPosition[threadNum]] = m_meshes.size();
    m_meshes.push_back(newMesh);
}

bool ClientWorld::buildMeshesForNewChunksWithNeighbours(int threadNum)
{
    bool meshBuilt = false;
    if (m_recentChunksBuilt.size() > 0) {
        m_unmeshedChunksMtx.lock();
        if (m_recentChunksBuilt.size() > 0) {
            IVec3 newChunkPosition = m_recentChunksBuilt.front();
            m_recentChunksBuilt.pop_front();
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
                    meshBuilt = true;
                }
            }
        }
        m_unmeshedChunksMtx.unlock();
    }
    // else if (!m_singleplayer && m_clientID > -1)
    // {
    //     requestMoreChunks();
    // }

    return meshBuilt;
}

uint8_t ClientWorld::shootRay(glm::vec3 startSubBlockPos, int* startBlockPosition, glm::vec3 direction, int* breakBlockCoords, int* placeBlockCoords) {
    //TODO: improve this to make it need less steps
    glm::vec3 rayPos = startSubBlockPos;
    int blockPos[3];
    int steps = 0;
    while (steps < 180) {
        rayPos += direction * 0.025f;
        for (int ii = 0; ii < 3; ii++)
            blockPos[ii] = floor(rayPos[ii]) + startBlockPosition[ii];
        if (!integratedServer.isChunkLoaded(Chunk::getChunkCoords(blockPos)))
            return 0;

        uint8_t blockType = integratedServer.chunkManager.getBlock(blockPos);
        if ((blockType != 0) && (blockType != 4)) {
            bool hit = true;
            for (int ii = 0; ii < 3; ii++) {
                if (rayPos[ii] < blockPos[ii] - startBlockPosition[ii] + integratedServer.getResourcePack().
                        getBlockData(blockType).model ->boundingBoxVertices[ii] + 0.5f
                    || rayPos[ii] > blockPos[ii] - startBlockPosition[ii] + integratedServer.getResourcePack().
                        getBlockData(blockType).model->boundingBoxVertices[ii + 15] + 0.5f) {
                    hit = false;
                }
            }
            if (hit) {
                for (int ii = 0; ii < 3; ii++)
                    breakBlockCoords[ii] = blockPos[ii];

                bool equal = true;
                while (equal) {
                    rayPos -= direction * 0.025f;
                    for (int ii = 0; ii < 3; ii++) {
                        placeBlockCoords[ii] = floor(rayPos[ii]) + startBlockPosition[ii];
                        equal &= placeBlockCoords[ii] == blockPos[ii];
                    }
                }

                for (int ii = 0; ii < 3; ii++)
                    placeBlockCoords[ii] = floor(rayPos[ii]) + startBlockPosition[ii];

                return blockType;
            }
        }
        steps++;
    }
    return 0;
}

void ClientWorld::replaceBlock(const IVec3& blockCoords, uint8_t blockType)
{
    IVec3 chunkPosition = Chunk::getChunkCoords(blockCoords);

    // LOG(std::to_string((uint32_t)integratedServer.chunkManager.getSkyLight(blockCoords)) + " sky light level\n");
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

    std::lock_guard<std::mutex> lock(m_meshesToUpdateMtx);
    for (auto& chunk : chunksToRemesh)
    {
        if (chunkHasNeighbours(chunk))
            m_meshesToUpdate.insert(chunk);
    }
}

void ClientWorld::setThreadWaiting(int threadNum, bool value)
{
    waitIfMeshesNeedUnloading(threadNum);
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
    for (auto& mesh : m_meshes)
    {
        unloadMesh(mesh);
    }
    m_meshArrayIndices.clear();
    m_meshes.resize(0);

    for (auto meshesToUnload : m_meshesToUnload)
    {
        for (auto& mesh : meshesToUnload)
            unloadMesh(mesh);

        meshesToUnload.clear();
    }
}

void ClientWorld::buildEntityMesh(const IVec3& playerBlockPos)
{
    float timeSinceLastTick = integratedServer.getTimeSinceLastTick();
    integratedServer.getEntityManager().getPhysicsEngine().extrapolateTransforms(timeSinceLastTick);
    GPUDynamicMeshBuffers& entityMesh = m_entityMeshes[
        m_renderer.getVulkanEngine().getFrameDataIndex()
    ];
    m_entityMeshManager.createBatch(
        playerBlockPos, reinterpret_cast<float*>(entityMesh.vertexBuffer.mappedData),
        reinterpret_cast<uint32_t*>(entityMesh.indexBuffer.mappedData), timeSinceLastTick
    );
    m_renderer.updateEntityMesh(m_entityMeshManager, entityMesh);
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
