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

#include "client/clientWorld.h"

#include "client/graphics/camera.h"
#include "client/graphics/renderer.h"
#include "core/pch.h"

#include "client/graphics/meshBuilder.h"
#include "core/constants.h"
#include "core/lighting.h"
#include "core/random.h"
#include "core/serverWorld.h"

namespace client {

static bool chunkMeshUploaded[32] = { false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false };
static bool unmeshCompleted = true;

ClientWorld::ClientWorld(uint16_t renderDistance, uint64_t seed, bool singleplayer,
    const IVec3& playerPos, ENetPeer* peer) : integratedServer(seed), m_singleplayer(singleplayer),
    m_peer(peer), m_clientID(-1), m_chunkRequestScheduled(true),
    m_meshManager(integratedServer, 1680000, 360000)
{
    m_renderDistance = renderDistance + 1;
    m_renderDiameter = m_renderDistance * 2 + 1;
    m_meshedChunksDistance = 0.0f;
    m_fogDistance = 0.0f;
    m_timeByDTs = 0.0;
    m_renderingFrame = false;
    m_renderThreadWaitingForArrIndicesVectors = false;

    IVec3 playerChunkPosition = Chunk::getChunkCoords(playerPos);
    m_playerChunkPosition[0] = playerChunkPosition.x;
    m_playerChunkPosition[1] = playerChunkPosition.y;
    m_playerChunkPosition[2] = playerChunkPosition.z;

    float minUnloadedChunkDistance = ((m_renderDistance + 1) * (m_renderDistance + 1));

    m_emptyIndexBuffer = new IndexBuffer();
    m_emptyVertexBuffer = new VertexBuffer();
    m_emptyVertexArray = new VertexArray(true);

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

void ClientWorld::renderWorld(Renderer mainRenderer, Shader& blockShader, Shader& waterShader,
    glm::mat4 viewMatrix, glm::mat4 projMatrix, int* playerBlockPosition, float aspectRatio, float
    fov, float skyLightIntensity, double DT) {
    Frustum viewFrustum = m_viewCamera.createViewFrustum(aspectRatio, fov, 0, 20);
    m_renderingFrame = true;
    float chunkCoordinates[3];

    blockShader.bind();
    blockShader.setUniformMat4f("u_proj", projMatrix);
    blockShader.setUniform1f("u_skyLightIntensity", skyLightIntensity);
    m_timeByDTs += DT;
    while (m_timeByDTs > (1.0/(double)constants::visualTPS)) {
        double fac = 0.006;
        double targetDistance = sqrt(m_meshedChunksDistance) - 1.2;
        m_fogDistance = m_fogDistance * (1.0 - fac) + targetDistance * fac * constants::CHUNK_SIZE;
        m_timeByDTs -= (1.0/(double)constants::visualTPS);
    }
    blockShader.setUniform1f("u_renderDistance", m_fogDistance);
    if (m_meshUpdates.size() > 0) {
        // auto tp1 = std::chrono::high_resolution_clock::now();
        while (m_meshUpdates.size() > 0) {
            doRenderThreadJobs();
        }
        // auto tp2 = std::chrono::high_resolution_clock::now();
        // std::cout << "waited " << std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count() << "us for chunks to remesh\n";
    }
    for (const auto& [chunkPosition, mesh] : m_meshes) {
        if (mesh.indexBuffer->getCount() > 0) {
            chunkCoordinates[0] = chunkPosition.x * constants::CHUNK_SIZE - playerBlockPosition[0];
            chunkCoordinates[1] = chunkPosition.y * constants::CHUNK_SIZE - playerBlockPosition[1];
            chunkCoordinates[2] = chunkPosition.z * constants::CHUNK_SIZE - playerBlockPosition[2];
            AABB aabb(glm::vec3(chunkCoordinates[0], chunkCoordinates[1], chunkCoordinates[2]), glm::vec3(chunkCoordinates[0] + constants::CHUNK_SIZE, chunkCoordinates[1] + constants::CHUNK_SIZE, chunkCoordinates[2] + constants::CHUNK_SIZE));
            if (aabb.isOnFrustum(viewFrustum)) {
                glm::mat4 modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(chunkCoordinates[0], chunkCoordinates[1], chunkCoordinates[2]));
                //update the MVP uniform
                blockShader.setUniformMat4f("u_modelView", viewMatrix * modelMatrix);
                mainRenderer.draw(*(mesh.vertexArray), *(mesh.indexBuffer), blockShader);
            }
            doRenderThreadJobs();
        }
    }

    // Render entities
    integratedServer.getEntityManager().extrapolateTransforms(integratedServer.getTimeSinceLastTick());
    m_meshManager.createBatch(playerBlockPosition);
    m_entityIndexBuffer->update(m_meshManager.indexBuffer.get(), m_meshManager.numIndices);
    m_entityVertexBuffer->update(m_meshManager.vertexBuffer.get(), m_meshManager.sizeOfVertices * sizeof(float));
    blockShader.setUniformMat4f("u_modelView", viewMatrix);
    mainRenderer.draw(*m_entityVertexArray, *m_entityIndexBuffer, blockShader);

    // Render water
    waterShader.bind();
    waterShader.setUniformMat4f("u_proj", projMatrix);
    waterShader.setUniform1f("u_skyLightIntensity", skyLightIntensity);
    waterShader.setUniform1f("u_renderDistance", m_fogDistance);
    glDisable(GL_CULL_FACE);
    for (const auto& [chunkPosition, mesh] : m_meshes) {
        if (mesh.waterIndexBuffer->getCount() > 0) {
            chunkCoordinates[0] = chunkPosition.x * constants::CHUNK_SIZE - playerBlockPosition[0];
            chunkCoordinates[1] = chunkPosition.y * constants::CHUNK_SIZE - playerBlockPosition[1];
            chunkCoordinates[2] = chunkPosition.z * constants::CHUNK_SIZE - playerBlockPosition[2];
            AABB aabb(glm::vec3(chunkCoordinates[0], chunkCoordinates[1], chunkCoordinates[2]), glm::vec3(chunkCoordinates[0] + constants::CHUNK_SIZE, chunkCoordinates[1] + constants::CHUNK_SIZE, chunkCoordinates[2] + constants::CHUNK_SIZE));
            if (aabb.isOnFrustum(viewFrustum)) {
                glm::mat4 modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(chunkCoordinates[0], chunkCoordinates[1], chunkCoordinates[2]));
                //update the MVP uniform
                waterShader.setUniformMat4f("u_modelView", viewMatrix * modelMatrix);
                mainRenderer.draw(*(mesh.waterVertexArray), *(mesh.waterIndexBuffer), waterShader);
            }
            doRenderThreadJobs();
        }
    }
    m_renderingFrame = false;
    glEnable(GL_CULL_FACE);
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
    m_unmeshedChunksMtx.lock();
    auto it = m_meshesToUpdate.begin();
    while (it != m_meshesToUpdate.end()) {
        unloadMesh(*it);
        m_meshUpdates.insert(*it);
        m_recentChunksBuilt.push(*it);
        it = m_meshesToUpdate.erase(it);
    }
    m_unmeshedChunksMtx.unlock();
}

void ClientWorld::updatePlayerPos(float playerX, float playerY, float playerZ) {
    IVec3 newPlayerChunkPosition = Chunk::getChunkCoords(IVec3(playerX, playerY, playerZ));
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
    int blockPosition[3] = { m_playerChunkPosition[0] * constants::CHUNK_SIZE,
                             m_playerChunkPosition[1] * constants::CHUNK_SIZE,
                             m_playerChunkPosition[2] * constants::CHUNK_SIZE };
    float subBlockPosition[3] = { playerX - blockPosition[0],
        playerY - blockPosition[1],
        playerZ - blockPosition[2] };

    bool readyToRelable = false;
    while (m_unmeshNeeded && (!readyToRelable)) {
        doRenderThreadJobs();
        //wait for all the mesh builder threads to finish their jobs
        for (int8_t threadNum = 0; threadNum < m_numChunkLoadingThreads; threadNum++) {
            readyToRelable |= !m_threadWaiting[threadNum];
        }
        readyToRelable = !readyToRelable;
    }

    integratedServer.updatePlayerPos(0, blockPosition, subBlockPosition, m_unmeshNeeded);

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
    m_unmeshedChunksMtx.lock();
    m_unmeshedChunks.insert(chunkPosition);
    m_recentChunksBuilt.push(chunkPosition);
    m_unmeshedChunksMtx.unlock();
}

void ClientWorld::unmeshChunks() {
    m_updatingPlayerChunkPosition[0] = m_newPlayerChunkPosition[0];
    m_updatingPlayerChunkPosition[1] = m_newPlayerChunkPosition[1];
    m_updatingPlayerChunkPosition[2] = m_newPlayerChunkPosition[2];
    //unload any meshes and chunks that are out of render distance
    float distance = 0;
    IVec3 lastChunkPosition;
    m_unmeshedChunksMtx.lock();
    for (const auto& [chunkPosition, mesh] : m_meshes) {
        if (distance > ((m_renderDistance + 0.999f) * (m_renderDistance + 0.999f))) {
            unloadMesh(lastChunkPosition);
        }
        distance = 0;
        distance += (chunkPosition.x - m_updatingPlayerChunkPosition[0]) * (chunkPosition.x - m_updatingPlayerChunkPosition[0]);
        distance += (chunkPosition.y - m_updatingPlayerChunkPosition[1]) * (chunkPosition.y - m_updatingPlayerChunkPosition[1]);
        distance += (chunkPosition.z - m_updatingPlayerChunkPosition[2]) * (chunkPosition.z - m_updatingPlayerChunkPosition[2]);
        lastChunkPosition = chunkPosition;
    }
    if (distance > ((m_renderDistance + 0.999f) * (m_renderDistance + 0.999f))) {
        unloadMesh(lastChunkPosition);
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
        // std::cout << "1\n";
    }
    else if (blockPosInChunk.x == constants::CHUNK_SIZE - 1) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(1, 0, 0));
        // std::cout << "2\n";
    }
    if (blockPosInChunk.y == 0) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(0, -1, 0));
        // std::cout << "3\n";
    }
    else if (blockPosInChunk.y == constants::CHUNK_SIZE - 1) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(0, 1, 0));
        // std::cout << "4\n";
    }
    if (blockPosInChunk.z == 0) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(0, 0, -1));
        // std::cout << "5\n";
    }
    else if (blockPosInChunk.z == constants::CHUNK_SIZE - 1) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + IVec3(0, 0, 1));
        // std::cout << "6\n";
    }
}

void ClientWorld::unloadMesh(const IVec3& chunkPosition) {
    auto it = m_meshes.find(chunkPosition);
    if (it == m_meshes.end()) {
        m_unmeshedChunks.insert(chunkPosition);
        return;
    }
    MeshData& mesh = it->second;
    if (mesh.indexBuffer->getCount() > 0) {
        delete mesh.vertexArray;
        delete mesh.vertexBuffer;
        delete mesh.indexBuffer;
    }

    if (mesh.waterIndexBuffer->getCount() > 0) {
        delete mesh.waterVertexArray;
        delete mesh.waterVertexBuffer;
        delete mesh.waterIndexBuffer;
    }

    m_meshes.erase(chunkPosition);
    m_unmeshedChunks.insert(chunkPosition);
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

    //if the chunk is empty fill the data with empty values to save interrupting the render thread
    if ((m_numChunkIndices[threadNum] == 0) && (m_numChunkWaterIndices[threadNum] == 0)) {
        auto it = m_meshUpdates.find(chunkPosition);
        if (it != m_meshUpdates.end()) {
            m_meshUpdates.erase(it);
        }
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
    //TODO: precalculate this VBL object
    //create vertex buffer layout
    VertexBufferLayout layout;
    layout.push<float>(3);
    layout.push<float>(2);
    layout.push<float>(1);
    layout.push<float>(1);

    VertexArray *va, *waterVa;
    VertexBuffer *vb, *waterVb;
    IndexBuffer *ib, *waterIb;

    if (m_numChunkIndices[threadNum] > 0) {
        va = new VertexArray;
        vb = new VertexBuffer(m_chunkVertices[threadNum], m_numChunkVertices[threadNum] * sizeof(float));
        va->addBuffer(*vb, layout);
        ib = new IndexBuffer(m_chunkIndices[threadNum], m_numChunkIndices[threadNum]);
    }
    else {
        va = m_emptyVertexArray;
        vb = m_emptyVertexBuffer;
        ib = m_emptyIndexBuffer;
    }

    if (m_numChunkWaterIndices[threadNum] > 0) {
        waterVa = new VertexArray;
        waterVb = new VertexBuffer(m_chunkWaterVertices[threadNum], m_numChunkWaterVertices[threadNum] * sizeof(float));
        waterVa->addBuffer(*waterVb, layout);
        waterIb = new IndexBuffer(m_chunkWaterIndices[threadNum], m_numChunkWaterIndices[threadNum]);
    }
    else {
        waterVa = m_emptyVertexArray;
        waterVb = m_emptyVertexBuffer;
        waterIb = m_emptyIndexBuffer;
    }

    m_renderThreadWaitingForArrIndicesVectorsMtx.lock();
    m_renderThreadWaitingForArrIndicesVectors = true;
    m_accessingArrIndicesVectorsMtx.lock();
    m_renderThreadWaitingForArrIndicesVectors = false;
    m_renderThreadWaitingForArrIndicesVectorsMtx.unlock();
    m_meshes[m_chunkPosition[threadNum]] = { va, vb, ib, waterVa, waterVb, waterIb };
    auto it = m_meshUpdates.find(m_chunkPosition[threadNum]);
    if (it != m_meshUpdates.end()) {
        m_meshUpdates.erase(it);
    }
    m_accessingArrIndicesVectorsMtx.unlock();
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
                        Chunk::s_checkingNeighbouringRelights.lock();
                        bool neighbourBeingRelit = true;
                        while (neighbourBeingRelit) {
                            neighbourBeingRelit = false;
                            for (uint32_t i = 0; i < 6; i++) {
                                neighbourBeingRelit |= integratedServer.chunkManager.getChunk(chunkPosition + s_neighbouringChunkOffsets[i]).isSkyLightBeingRelit();
                            }
                            if (neighbourBeingRelit) {
                                std::this_thread::sleep_for(std::chrono::microseconds(100));
                            }
                        }
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
                        chunk.setSkyLightBeingRelit(false);
                    }
                    addChunkMesh(chunkPosition, threadNum);
                    m_unmeshedChunksMtx.lock();
                }
            }
        }
        m_unmeshedChunksMtx.unlock();
    }
    else if (!m_singleplayer && m_clientID > -1)
    {
        requestMoreChunks();
    }
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

    // std::cout << (uint32_t)m_integratedServer.getBlockLight(blockCoords) << " block light level\n";
    uint8_t originalBlockType = integratedServer.chunkManager.getBlock(blockCoords);
    integratedServer.chunkManager.setBlock(blockCoords, blockType);

    std::vector<IVec3> chunksToRemesh;
    addChunksToRemesh(chunksToRemesh, blockCoords, chunkPosition);

    auto tp1 = std::chrono::high_resolution_clock::now();
    Lighting::relightChunksAroundBlock(blockCoords, chunkPosition, originalBlockType, blockType,
        chunksToRemesh, integratedServer.chunkManager.getWorldChunks(), integratedServer.getResourcePack());
    auto tp2 = std::chrono::high_resolution_clock::now();
    // std::cout << chunksToRemesh.size() << " chunks remeshed\n";
    // std::cout << "relight took " << std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count() << "us\n";

    m_accessingArrIndicesVectorsMtx.lock();
    while (m_renderThreadWaitingForArrIndicesVectors) {
        m_accessingArrIndicesVectorsMtx.unlock();
        m_renderThreadWaitingForArrIndicesVectorsMtx.lock();
        m_accessingArrIndicesVectorsMtx.lock();
        m_renderThreadWaitingForArrIndicesVectorsMtx.unlock();
    }

    for (auto& chunk : chunksToRemesh) {
        if (chunkHasNeighbours(chunk)) {
            m_meshesToUpdate.insert(chunk);
        }
    }
    m_accessingArrIndicesVectorsMtx.unlock();
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

void ClientWorld::initialiseEntityRenderBuffers()
{
    VertexBufferLayout entityLayout;
    entityLayout.push<float>(3);
    entityLayout.push<float>(2);
    entityLayout.push<float>(1);
    entityLayout.push<float>(1);
    m_entityVertexArray = new VertexArray();
    m_entityVertexBuffer = new VertexBuffer(m_meshManager.vertexBuffer.get(), 0, true);
    m_entityIndexBuffer = new IndexBuffer(m_meshManager.indexBuffer.get(), 0, true);
    m_entityVertexArray->addBuffer(*m_entityVertexBuffer, entityLayout);
}

void ClientWorld::deinitialiseEntityRenderBuffers()
{
    delete m_entityVertexArray;
    delete m_entityVertexBuffer;
    delete m_entityIndexBuffer;
}

void ClientWorld::requestMoreChunks()
{
    ServerPlayer& player = integratedServer.getPlayer(0);
    if (player.updateChunkLoadingTarget() || m_chunkRequestScheduled)
    {
        // std::cout << "Requesting " << player.getChunkLoadingTarget() << " chunks\n";
        Packet<int64_t, 2> payload(m_clientID, PacketType::ChunkRequest, 2);
        payload[0] = player.incrementNumChunkRequests();
        payload[1] = player.getChunkLoadingTarget();
        ENetPacket* packet = enet_packet_create(
            (const void*)(&payload), payload.getSize(), ENET_PACKET_FLAG_RELIABLE
        );
        enet_peer_send(m_peer, 0, packet);
        m_chunkRequestScheduled = false;
    }
}

}  // namespace client
