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

#include "core/pch.h"
#include <time.h>
#include <random>

#include "client/graphics/meshBuilder.h"
#include "core/constants.h"
#include "core/lighting.h"
#include "core/random.h"
#include "core/serverWorld.h"
#include "core/terrainGen.h"

namespace client {

static bool chunkMeshUploaded[32] = { false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false, false, false,
    false, false, false, false, false, false, false, false, false, false, false };
static bool unmeshCompleted = true;

ClientWorld::ClientWorld(unsigned short renderDistance, unsigned long long seed, bool singleplayer,
    const Position& playerPos) : m_singleplayer(singleplayer), m_integratedServer(seed) {
    //seed the random number generator and the simplex noise
    m_seed = seed;
    PCG_SeedRandom32(m_seed);
    seedNoise();

    m_renderDistance = renderDistance + 1;
    m_renderDiameter = m_renderDistance * 2 + 1;
    m_meshedChunksDistance = 0.0f;
    m_fogDistance = 0.0f;
    m_timeByDTs = 0.0;
    m_mouseCalls = 0;
    m_renderingFrame = false;
    m_renderThreadWaitingForArrIndicesVectors = false;

    m_playerChunkPosition[0] = std::floor((float)playerPos.x / constants::CHUNK_SIZE);
    m_playerChunkPosition[1] = std::floor((float)playerPos.y / constants::CHUNK_SIZE);
    m_playerChunkPosition[2] = std::floor((float)playerPos.z / constants::CHUNK_SIZE);
    
    float minUnloadedChunkDistance = ((m_renderDistance + 1) * (m_renderDistance + 1));

    m_emptyIndexBuffer = new IndexBuffer();
    m_emptyVertexBuffer = new VertexBuffer();
    m_emptyVertexArray = new VertexArray(true);
    
    m_numChunkLoadingThreads = m_integratedServer.getNumChunkLoaderThreads() - 1;
    
    //allocate arrays on the heap for the mesh to be built
    //do this now so that the same array can be reused for each chunk
    m_numChunkVertices = new unsigned int[m_numChunkLoadingThreads];
    m_numChunkIndices = new unsigned int[m_numChunkLoadingThreads];
    m_numChunkWaterVertices = new unsigned int[m_numChunkLoadingThreads];
    m_numChunkWaterIndices = new unsigned int[m_numChunkLoadingThreads];
    m_chunkVertices = new float*[m_numChunkLoadingThreads];
    m_chunkIndices = new unsigned int*[m_numChunkLoadingThreads];
    m_chunkWaterVertices = new float*[m_numChunkLoadingThreads];
    m_chunkWaterIndices = new unsigned int*[m_numChunkLoadingThreads];
    m_chunkPosition = new Position[m_numChunkLoadingThreads];
    m_chunkMeshReady = new bool[m_numChunkLoadingThreads];
    m_chunkMeshReadyCV = new std::condition_variable[m_numChunkLoadingThreads];
    m_chunkMeshReadyMtx = new std::mutex[m_numChunkLoadingThreads];
    m_threadWaiting = new bool[m_numChunkLoadingThreads];
    m_unmeshNeeded = false;
    
    for (int i = 0; i < m_numChunkLoadingThreads; i++) {
        m_chunkVertices[i] = new float[12 * 6 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        m_chunkIndices[i] = new unsigned int[18 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        m_chunkWaterVertices[i] = new float[12 * 6 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        m_chunkWaterIndices[i] = new unsigned int[18 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        m_chunkMeshReady[i] = false;
        m_threadWaiting[i] = false;
    }
    int playerBlockPosition[3] = { playerPos.x, playerPos.y, playerPos.z };
    float playerSubBlockPosition[3] = { 0.0f, 0.0f, 0.0f };
    m_integratedServer.addPlayer(playerBlockPosition, playerSubBlockPosition, m_renderDistance);

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

void ClientWorld::renderChunks(Renderer mainRenderer, Shader& blockShader, Shader& waterShader,
    glm::mat4 viewMatrix, glm::mat4 projMatrix, int* playerBlockPosition, float aspectRatio, float
    fov, float skyLightIntensity, double DT) {
    Frustum viewFrustum = m_viewCamera->createViewFrustum(aspectRatio, fov, 0, 20);
    m_renderingFrame = true;
    float chunkCoordinates[3];
    //float centredChunkCoordinates[3];
    //render blocks
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
        auto tp1 = std::chrono::high_resolution_clock::now();
        while (m_meshUpdates.size() > 0) {
            doRenderThreadJobs();
        }
        auto tp2 = std::chrono::high_resolution_clock::now();
        std::cout << "waited " << std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count() << "us for chunks to remesh\n";
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
                mesh.vertexArray->bind();
                mainRenderer.draw(*(mesh.vertexArray), *(mesh.indexBuffer), blockShader);
            }
            doRenderThreadJobs();
        }
    }
    if (m_unmeshNeeded) {
        unmeshCompleted = false;
    }
    auto tp2 = std::chrono::high_resolution_clock::now();
    //render water
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
                mesh.waterVertexArray->bind();
                mainRenderer.draw(*(mesh.waterVertexArray), *(mesh.waterIndexBuffer), waterShader);
            }
            doRenderThreadJobs();
        }
    }
    m_renderingFrame = false;
    glEnable(GL_CULL_FACE);
    m_mouseCalls += 100;
}

void ClientWorld::doRenderThreadJobs() {
    for (char threadNum = 0; threadNum < m_numChunkLoadingThreads; threadNum++) {
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
    //process the mouse input occasionally
    if (++m_mouseCalls > 500) {
        processMouseInput();
        m_mouseCalls = 0;
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
    m_newPlayerChunkPosition[0] = std::floor((float)playerX / constants::CHUNK_SIZE);
    m_newPlayerChunkPosition[1] = std::floor((float)playerY / constants::CHUNK_SIZE);
    m_newPlayerChunkPosition[2] = std::floor((float)playerZ / constants::CHUNK_SIZE);
    // m_newPlayerChunkPosition[0] = -1 * (playerX < 0) + playerX / constants::CHUNK_SIZE;
    // m_newPlayerChunkPosition[1] = -1 * (playerY < 0) + playerY / constants::CHUNK_SIZE;
    // m_newPlayerChunkPosition[2] = -1 * (playerZ < 0) + playerZ / constants::CHUNK_SIZE;

    unmeshCompleted = (m_playerChunkPosition[0] == m_newPlayerChunkPosition[0])
        && (m_playerChunkPosition[1] == m_newPlayerChunkPosition[1])
        && (m_playerChunkPosition[2] == m_newPlayerChunkPosition[2]);
    m_unmeshNeeded = !unmeshCompleted;

    for (char i = 0; i < 3; i++) {
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
        for (char threadNum = 0; threadNum < m_numChunkLoadingThreads; threadNum++) {
            readyToRelable |= !m_threadWaiting[threadNum];
        }
        readyToRelable = !readyToRelable;
    }

    m_integratedServer.updatePlayerPos(0, blockPosition, subBlockPosition, m_unmeshNeeded);

    if (m_unmeshNeeded) {
        unmeshChunks();
        unmeshCompleted = true;
        m_unmeshNeeded = false;
        // lock release 
        std::lock_guard<std::mutex> lock(m_unmeshNeededMtx);
        // notify consumer when done
        m_unmeshNeededCV.notify_all();
    }
}

void ClientWorld::loadChunksAroundPlayerSingleplayer(char threadNum) {
    while (m_unmeshNeeded && (m_meshUpdates.size() == 0)) {
        m_threadWaiting[threadNum] = true;
        // locking 
        std::unique_lock<std::mutex> lock(m_unmeshNeededMtx);
        // waiting 
        m_unmeshNeededCV.wait(lock, [] { return unmeshCompleted; });
        m_threadWaiting[threadNum] = false;
    }
    if (m_meshUpdates.empty()) {
        Position chunkPosition;
        if (m_integratedServer.loadChunk(&chunkPosition)) {
            m_unmeshedChunksMtx.lock();
            m_unmeshedChunks.insert(chunkPosition);
            m_recentChunksBuilt.push(chunkPosition);
            m_unmeshedChunksMtx.unlock();
        }
    }
    buildMeshesForNewChunksWithNeighbours(threadNum);
}

void ClientWorld::loadChunksAroundPlayerMultiplayer(char threadNum) {
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

void ClientWorld::loadChunkFromPacket(Packet<unsigned char, 9 * constants::CHUNK_SIZE *
    constants::CHUNK_SIZE * constants::CHUNK_SIZE>& payload) {
    Position chunkPosition;
    m_integratedServer.loadChunkFromPacket(payload, chunkPosition);
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
    Position lastChunkPosition;
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
        if (!m_integratedServer.chunkLoaded(*it)) {
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

bool ClientWorld::chunkHasNeighbours(const Position& chunkPosition) {
    for (unsigned char i = 0; i < 27; i++) {
        // std::cout << (chunkPosition + m_neighbouringChunkIncludingDiaganalOffsets[i]).x << ", "
        //            << (chunkPosition + m_neighbouringChunkIncludingDiaganalOffsets[i]).y << ", "
        //            << (chunkPosition + m_neighbouringChunkIncludingDiaganalOffsets[i]).z << "\n";
        if (!(m_integratedServer.chunkLoaded(chunkPosition + m_neighbouringChunkIncludingDiaganalOffsets[i]))) {
            return false;
        }
    }

    return true;
}

void ClientWorld::addChunksToRemesh(std::vector<Position>& chunksToRemesh, const Position&
    modifiedBlockPos, const Position& modifiedBlockChunk) {
    chunksToRemesh.push_back(modifiedBlockChunk);
    Position blockPosInChunk = modifiedBlockPos - modifiedBlockChunk * constants::CHUNK_SIZE;
    if (blockPosInChunk.x == 0) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + Position(-1, 0, 0));
        std::cout << "1\n";
    }
    else if (blockPosInChunk.x == constants::CHUNK_SIZE - 1) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + Position(1, 0, 0));
        std::cout << "2\n";
    }
    if (blockPosInChunk.y == 0) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + Position(0, -1, 0));
        std::cout << "3\n";
    }
    else if (blockPosInChunk.y == constants::CHUNK_SIZE - 1) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + Position(0, 1, 0));
        std::cout << "4\n";
    }
    if (blockPosInChunk.z == 0) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + Position(0, 0, -1));
        std::cout << "5\n";
    }
    else if (blockPosInChunk.z == constants::CHUNK_SIZE - 1) {
        chunksToRemesh.emplace_back(modifiedBlockChunk + Position(0, 0, 1));
        std::cout << "6\n";
    }
}

void ClientWorld::unloadMesh(const Position& chunkPosition) {
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

void ClientWorld::addChunkMesh(const Position& chunkPosition, char threadNum) {
    int chunkCoords[3] = { chunkPosition.x, chunkPosition.y, chunkPosition.z };

    //generate the mesh
    MeshBuilder(m_integratedServer.getChunk(chunkPosition), m_integratedServer, m_chunkVertices[threadNum], &m_numChunkVertices[threadNum], m_chunkIndices[threadNum], &m_numChunkIndices[threadNum], m_chunkWaterVertices[threadNum], &m_numChunkWaterVertices[threadNum], m_chunkWaterIndices[threadNum], &m_numChunkWaterIndices[threadNum]).buildMesh();

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

void ClientWorld::uploadChunkMesh(char threadNum) {
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

void ClientWorld::buildMeshesForNewChunksWithNeighbours(char threadNum) {
    if (m_recentChunksBuilt.size() > 0) {
        m_unmeshedChunksMtx.lock();
        if (m_recentChunksBuilt.size() > 0) {
            Position newChunkPosition = m_recentChunksBuilt.front();
            m_recentChunksBuilt.pop();
            for (int i = 0; i < 27; i++) {
                Position chunkPosition = newChunkPosition + m_neighbouringChunkIncludingDiaganalOffsets[i];
                if (m_unmeshedChunks.contains(chunkPosition) && chunkHasNeighbours(chunkPosition)) {
                    m_unmeshedChunks.erase(chunkPosition);
                    m_unmeshedChunksMtx.unlock();
                    Chunk& chunk = m_integratedServer.getChunk(chunkPosition);
                    if (!chunk.isSkylightUpToDate()) {
                        Chunk::s_checkingNeighbouringRelights.lock();
                        bool neighbourBeingRelit = true;
                        while (neighbourBeingRelit) {
                            neighbourBeingRelit = false;
                            for (unsigned int i = 0; i < 6; i++) {
                                neighbourBeingRelit |= m_integratedServer.getChunk(chunkPosition + s_neighbouringChunkOffsets[i]).isSkyBeingRelit();
                            }
                            if (neighbourBeingRelit) {
                                std::this_thread::sleep_for(std::chrono::microseconds(100));
                            }
                        }
                        chunk.setSkylightBeingRelit(true);
                        chunk.clearSkyLight();
                        bool neighbouringChunksToRelight[6];
                        bool chunksToRemesh[7];
                        Lighting::propagateSkyLight(chunkPosition, m_integratedServer.
                            getWorldChunks(), neighbouringChunksToRelight, chunksToRemesh,
                            m_integratedServer.getResourcePack());
                        chunk.setSkylightBeingRelit(false);
                    }
                    addChunkMesh(chunkPosition, threadNum);
                    m_unmeshedChunksMtx.lock();
                }
            }
        }
        m_unmeshedChunksMtx.unlock();
    }
}

unsigned char ClientWorld::shootRay(glm::vec3 startSubBlockPos, int* startBlockPosition, glm::vec3 direction, int* breakBlockCoords, int* placeBlockCoords) {
    //TODO: improve this to make it need less steps
    glm::vec3 rayPos = startSubBlockPos;
    int blockPos[3];
    int steps = 0;
    while (steps < 180) {
        rayPos += direction * 0.025f;
        for (unsigned char ii = 0; ii < 3; ii++) {
            blockPos[ii] = floor(rayPos[ii]) + startBlockPosition[ii];
        }
        unsigned char blockType = getBlock(blockPos);
        if ((blockType != 0) && (blockType != 4)) {
            bool hit = true;
            for (unsigned char ii = 0; ii < 3; ii++) {
                if (rayPos[ii] < blockPos[ii] - startBlockPosition[ii] + m_integratedServer.getResourcePack().getBlockData(blockType).model
                    ->boundingBoxVertices[ii] || rayPos[ii] > blockPos[ii] - startBlockPosition[ii] + m_integratedServer.getResourcePack().
                    getBlockData(blockType).model->boundingBoxVertices[ii + 15]) {
                    hit = false;
                }
            }
            if (hit) {
                for (unsigned char ii = 0; ii < 3; ii++) {
                    breakBlockCoords[ii] = blockPos[ii];
                }
                
                bool equal = true;
                while (equal) {
                    rayPos -= direction * 0.025f;
                    for (unsigned char ii = 0; ii < 3; ii++) {
                        placeBlockCoords[ii] = floor(rayPos[ii]) + startBlockPosition[ii];
                        equal &= placeBlockCoords[ii] == blockPos[ii];
                    }
                }

                for (unsigned char ii = 0; ii < 3; ii++) {
                    placeBlockCoords[ii] = floor(rayPos[ii]) + startBlockPosition[ii];
                }
                return blockType;
            }
        }
        steps++;
    }
    return 0;
}

void ClientWorld::replaceBlock(const Position& blockCoords, unsigned char blockType) {
    Position chunkPosition;
    chunkPosition.x = std::floor((float)blockCoords.x / constants::CHUNK_SIZE);
    chunkPosition.y = std::floor((float)blockCoords.y / constants::CHUNK_SIZE);
    chunkPosition.z = std::floor((float)blockCoords.z / constants::CHUNK_SIZE);
    
    std::cout << (unsigned int)m_integratedServer.getSkyLight(blockCoords) << " light level\n";
    unsigned char originalBlockType = m_integratedServer.getBlock(blockCoords);
    m_integratedServer.setBlock(blockCoords, blockType);

    std::vector<Position> chunksToRemesh;
    addChunksToRemesh(chunksToRemesh, blockCoords, chunkPosition);

    auto tp1 = std::chrono::high_resolution_clock::now();
    Lighting::relightChunksAroundBlock(blockCoords, chunkPosition, originalBlockType, blockType,
        chunksToRemesh, m_integratedServer.getWorldChunks(), m_integratedServer.getResourcePack());
    auto tp2 = std::chrono::high_resolution_clock::now();
    std::cout << chunksToRemesh.size() << " chunks remeshed\n";
    std::cout << "relight took " << std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count() << "us\n";

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

void ClientWorld::processMouseInput() {
    auto end = std::chrono::steady_clock::now();
    double currentTime = (double)std::chrono::duration_cast<std::chrono::microseconds>(end - m_startTime).count() / 1000;
    if (*m_lastMousePoll == 0.0f) {
        *m_lastMousePoll = currentTime;
        return;
    }
    double DT = (currentTime - (*m_lastMousePoll)) * 0.001;
    if (DT < 0.001) {
        return;
    }
    *m_lastMousePoll = currentTime;

    int localCursorPosition[2];
    SDL_PumpEvents();
    Uint32 mouseState = SDL_GetMouseState(&localCursorPosition[0], &localCursorPosition[1]);

    //mouse input
    if (*m_playing) {
        if (*m_lastPlaying) {
            *m_yaw += (localCursorPosition[0] - m_lastMousePos[0]) * 0.05f;
            *m_pitch -= (localCursorPosition[1] - m_lastMousePos[1]) * 0.05f;
            if (*m_pitch <= -90.0f) {
                *m_pitch = -89.999f;
            }
            else if (*m_pitch >= 90.0f) {
                *m_pitch = 89.999f;
            }

            m_viewCamera->updateRotationVectors(*m_yaw, *m_pitch);
        }
        if ((abs(localCursorPosition[0] - (int)m_windowDimensions[0] / 2) > m_windowDimensions[0] / 16)
            || (abs(localCursorPosition[1] - (int)m_windowDimensions[1] / 2) > m_windowDimensions[1] / 16)) {
            SDL_WarpMouseInWindow(m_window, m_windowDimensions[0] / 2, m_windowDimensions[1] / 2);
            m_lastMousePos[0] = m_windowDimensions[0] / 2;
            m_lastMousePos[1] = m_windowDimensions[1] / 2;
        }
        else {
            m_lastMousePos[0] = localCursorPosition[0];
            m_lastMousePos[1] = localCursorPosition[1];
        }
    }
}

void ClientWorld::setMouseData(double* lastMousePoll,
    bool* playing,
    bool* lastPlaying,
    float* yaw,
    float* pitch,
    int* lastMousePos,
    Camera* viewCamera,
    SDL_Window* window,
    unsigned int* windowDimensions) {
    m_lastMousePoll = lastMousePoll;
    m_playing = playing;
    m_lastPlaying = lastPlaying;
    m_yaw = yaw;
    m_pitch = pitch;
    m_lastMousePos = lastMousePos;
    m_viewCamera = viewCamera;
    m_window = window;
    m_windowDimensions = windowDimensions;
    m_startTime = std::chrono::steady_clock::now();
}

void ClientWorld::setThreadWaiting(unsigned char threadNum, bool value) {
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

}  // namespace client