#include "world.h"
#include "constants.h"
#include "random.h"

#include <thread>
#include <cmath>
#include <time.h>
#include <iostream>
#include <random>
#include <algorithm>

bool chunkMeshUploaded[4] = { false, false, false, false };
bool relableCompleted = false;

world::world(unsigned short renderDistance, unsigned long long seed) {
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
    m_numMeshesUnloaded = m_numChunksUnloaded = 0;
    m_renderingFrame = false;
    m_renderThreadWaitingForArrIndicesVectors = false;
    m_numMeshUpdates = 0;

    int playerPosition[3] = { 0, 0, 0 };
    int chunkCoords[3];
    m_numChunks = m_renderDiameter * m_renderDiameter * m_renderDiameter;
    float minUnloadedChunkDistance = ((m_renderDistance + 1) * (m_renderDistance + 1));
    m_numActualChunks = 0;
    for (unsigned int i = 0; i < m_numChunks; i++) {
        getChunkCoords(chunkCoords, i, playerPosition);
        if ((chunkCoords[0] * chunkCoords[0] + chunkCoords[1] * chunkCoords[1] + chunkCoords[2] * chunkCoords[2]) < minUnloadedChunkDistance) {
            m_numActualChunks++;
        }
    }

    m_loadedChunks = new bool[m_numChunks];
    m_loadingChunks = new bool[m_numChunks];
    m_chunkDistances = new float[m_numChunks];
    m_chunkArrayIndices = new unsigned int[m_numChunks];
    m_meshUpdates = new bool[m_numChunks];

    m_emptyIndexBuffer = new indexBuffer();
    m_emptyVertexBuffer = new vertexBuffer();
    m_emptyVertexArray = new vertexArray(true);

    //create space for chunks on the heap so the memory does not have to be allocated during gameplay
    m_chunks = new chunk[m_numActualChunks];

    m_worldInfo.worldChunks = m_chunks;
    m_worldInfo.chunkArrayIndices = m_chunkArrayIndices;
    m_worldInfo.playerChunkPosition = m_playerChunkPosition;
    m_worldInfo.renderDistance = m_renderDistance;
    m_worldInfo.renderDiameter = m_renderDiameter;
    m_numRelights = 0;
    m_worldInfo.numRelights = &m_numRelights;
    m_worldInfo.seed = m_seed;

    //allocate arrays on the heap for the mesh to be built
    //do this now so that the same array can be reused for each chunk
    m_numChunkLoadingThreads = std::thread::hardware_concurrency() - 1;
    if (!m_numChunkLoadingThreads) {
        m_numChunkLoadingThreads = 1;
    }
    else if (m_numChunkLoadingThreads > 4) {
        m_numChunkLoadingThreads = 4;
    }
    m_numChunkVertices = new unsigned int[m_numChunkLoadingThreads];
    m_numChunkIndices = new unsigned int[m_numChunkLoadingThreads];
    m_numChunkWaterVertices = new unsigned int[m_numChunkLoadingThreads];
    m_numChunkWaterIndices = new unsigned int[m_numChunkLoadingThreads];
    m_chunkVertices = new float*[m_numChunkLoadingThreads];
    m_chunkIndices = new unsigned int*[m_numChunkLoadingThreads];
    m_chunkWaterVertices = new float*[m_numChunkLoadingThreads];
    m_chunkWaterIndices = new unsigned int*[m_numChunkLoadingThreads];
    m_chunkVectorIndex = new unsigned int[m_numChunkLoadingThreads];
    m_chunkMeshReady = new bool[m_numChunkLoadingThreads];
    m_chunkMeshReadyCV = new std::condition_variable[m_numChunkLoadingThreads];
    m_chunkMeshReadyMtx = new std::mutex[m_numChunkLoadingThreads];
    m_threadWaiting = new bool[m_numChunkLoadingThreads];
    m_relableNeeded = true;
    
    for (int i = 0; i < m_numChunkLoadingThreads; i++) {
        m_chunkVertices[i] = new float[12 * 6 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        m_chunkIndices[i] = new unsigned int[18 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        m_chunkWaterVertices[i] = new float[12 * 6 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        m_chunkWaterIndices[i] = new unsigned int[18 * constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE];
        m_chunkMeshReady[i] = false;
        m_threadWaiting[i] = false;
    }

    //populate the list of chunk distances with pre-calculated values, so they don't have to be calculated during gameplay
    for (unsigned int i = 0; i < m_numChunks; i++) {
        getChunkCoords(chunkCoords, i, playerPosition);
        m_chunkDistances[i] = chunkCoords[0] * chunkCoords[0] + chunkCoords[1] * chunkCoords[1] + chunkCoords[2] * chunkCoords[2];
        //set the chunk to a blank chunk
        m_loadedChunks[i] = false;
        m_loadingChunks[i] = false;
        m_meshUpdates[i] = false;
    }
    for (unsigned int i = 0; i < m_numActualChunks; i++) {
        m_chunks[i].setWorldInfo(m_worldInfo);
    }

    //calculate the offset chunk numbers for the neighbouring chunks
    m_neighbouringChunkNumberOffets[0] = -(m_renderDiameter * m_renderDiameter);
    m_neighbouringChunkNumberOffets[1] = -m_renderDiameter;
    m_neighbouringChunkNumberOffets[2] = -1;
    m_neighbouringChunkNumberOffets[3] = 1;
    m_neighbouringChunkNumberOffets[4] = m_renderDiameter;
    m_neighbouringChunkNumberOffets[5] = (m_renderDiameter * m_renderDiameter);

    m_neighbouringChunkIncludingDiaganalOffsets[0]  = -(m_renderDiameter * m_renderDiameter) - m_renderDiameter - 1;
    m_neighbouringChunkIncludingDiaganalOffsets[1]  = -(m_renderDiameter * m_renderDiameter) - m_renderDiameter;
    m_neighbouringChunkIncludingDiaganalOffsets[2]  = -(m_renderDiameter * m_renderDiameter) - m_renderDiameter + 1;
    m_neighbouringChunkIncludingDiaganalOffsets[3]  = -(m_renderDiameter * m_renderDiameter) - 1;
    m_neighbouringChunkIncludingDiaganalOffsets[4]  = -(m_renderDiameter * m_renderDiameter);
    m_neighbouringChunkIncludingDiaganalOffsets[5]  = -(m_renderDiameter * m_renderDiameter) + 1;
    m_neighbouringChunkIncludingDiaganalOffsets[6]  = -(m_renderDiameter * m_renderDiameter) + m_renderDiameter - 1;
    m_neighbouringChunkIncludingDiaganalOffsets[7]  = -(m_renderDiameter * m_renderDiameter) + m_renderDiameter;
    m_neighbouringChunkIncludingDiaganalOffsets[8]  = -(m_renderDiameter * m_renderDiameter) + m_renderDiameter + 1;
    m_neighbouringChunkIncludingDiaganalOffsets[9]  = -m_renderDiameter - 1;
    m_neighbouringChunkIncludingDiaganalOffsets[10] = -m_renderDiameter;
    m_neighbouringChunkIncludingDiaganalOffsets[11] = -m_renderDiameter + 1;
    m_neighbouringChunkIncludingDiaganalOffsets[12] = -1;
    m_neighbouringChunkIncludingDiaganalOffsets[13] = 0;
    m_neighbouringChunkIncludingDiaganalOffsets[14] = 1;
    m_neighbouringChunkIncludingDiaganalOffsets[15] = m_renderDiameter - 1;
    m_neighbouringChunkIncludingDiaganalOffsets[16] = m_renderDiameter;
    m_neighbouringChunkIncludingDiaganalOffsets[17] = m_renderDiameter + 1;
    m_neighbouringChunkIncludingDiaganalOffsets[18] = (m_renderDiameter * m_renderDiameter) - m_renderDiameter - 1;
    m_neighbouringChunkIncludingDiaganalOffsets[19] = (m_renderDiameter * m_renderDiameter) - m_renderDiameter;
    m_neighbouringChunkIncludingDiaganalOffsets[20] = (m_renderDiameter * m_renderDiameter) - m_renderDiameter + 1;
    m_neighbouringChunkIncludingDiaganalOffsets[21] = (m_renderDiameter * m_renderDiameter) - 1;
    m_neighbouringChunkIncludingDiaganalOffsets[22] = (m_renderDiameter * m_renderDiameter);
    m_neighbouringChunkIncludingDiaganalOffsets[23] = (m_renderDiameter * m_renderDiameter) + 1;
    m_neighbouringChunkIncludingDiaganalOffsets[24] = (m_renderDiameter * m_renderDiameter) + m_renderDiameter - 1;
    m_neighbouringChunkIncludingDiaganalOffsets[25] = (m_renderDiameter * m_renderDiameter) + m_renderDiameter;
    m_neighbouringChunkIncludingDiaganalOffsets[26] = (m_renderDiameter * m_renderDiameter) + m_renderDiameter + 1;
}

void world::renderChunks(renderer mainRenderer, shader& blockShader, shader& waterShader, glm::mat4 viewMatrix, glm::mat4 projMatrix, int* playerBlockPosition, float aspectRatio, float fov, double DT) {
    if (m_chunkIndexBuffers.size() != m_meshedChunkArrayIndices.size()) {
        std::cout << "bad\n";
    }
    //else {
    //    cout << "good\n";
    //}
    frustum viewFrustum = m_viewCamera->createViewFrustum(aspectRatio, fov, 0, 20);
    m_renderingFrame = true;
    int chunkPosition[3];
    float chunkCoordinates[3];
    //float centredChunkCoordinates[3];
    //render blocks
    blockShader.bind();
    blockShader.setUniformMat4f("u_proj", projMatrix);
    m_timeByDTs += DT;
    while (m_timeByDTs > (1.0/(double)constants::visualTPS)) {
        constexpr double fac = 0.016;
        m_fogDistance = m_fogDistance * (1.0 - fac) + (sqrt(m_meshedChunksDistance) - 2.0) * fac * constants::CHUNK_SIZE;
        m_timeByDTs -= (1.0/(double)constants::visualTPS);
    }
    blockShader.setUniform1f("u_renderDistance", m_fogDistance);
    unsigned int meshNum = 0;
    while (meshNum < m_meshedChunkArrayIndices.size()) {
        if (m_chunkIndexBuffers[meshNum]->getCount() > 0) {
            m_chunks[m_meshedChunkArrayIndices[meshNum]].getChunkPosition(chunkPosition);
            for (unsigned char ii = 0; ii < 3; ii++) {
                chunkCoordinates[ii] = chunkPosition[ii] * constants::CHUNK_SIZE - playerBlockPosition[ii];
            }
            AABB aabb(glm::vec3(chunkCoordinates[0], chunkCoordinates[1], chunkCoordinates[2]), glm::vec3(chunkCoordinates[0] + constants::CHUNK_SIZE, chunkCoordinates[1] + constants::CHUNK_SIZE, chunkCoordinates[2] + constants::CHUNK_SIZE));
            if (aabb.isOnFrustum(viewFrustum)) {
                glm::mat4 modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(chunkCoordinates[0], chunkCoordinates[1], chunkCoordinates[2]));
                //update the MVP uniform
                blockShader.setUniformMat4f("u_modelView", viewMatrix * modelMatrix);
                m_chunkVertexArrays[meshNum]->bind();
                mainRenderer.draw(*m_chunkVertexArrays[meshNum], *m_chunkIndexBuffers[meshNum], blockShader);
                doRenderThreadJobs();
            }
        }
        meshNum++;
	}
    //ensure that all meshes have been reloaded before moving on to water
    if (m_numMeshUpdates > 0) {
        auto tp1 = std::chrono::high_resolution_clock::now();
        while (m_numMeshUpdates > 0) {
            //cout << "m_numMeshUpdates > 0: " << m_numMeshUpdates << "\n";
            doRenderThreadJobs();
        }
        while (meshNum < m_meshedChunkArrayIndices.size()) {
            if (m_chunkIndexBuffers[meshNum]->getCount() > 0) {
                m_chunks[m_meshedChunkArrayIndices[meshNum]].getChunkPosition(chunkPosition);
                for (unsigned char ii = 0; ii < 3; ii++) {
                    chunkCoordinates[ii] = chunkPosition[ii] * constants::CHUNK_SIZE - playerBlockPosition[ii];
                }
                AABB aabb(glm::vec3(chunkCoordinates[0], chunkCoordinates[1], chunkCoordinates[2]), glm::vec3(chunkCoordinates[0] + constants::CHUNK_SIZE, chunkCoordinates[1] + constants::CHUNK_SIZE, chunkCoordinates[2] + constants::CHUNK_SIZE));
                if (aabb.isOnFrustum(viewFrustum)) {
                    glm::mat4 modelMatrix = glm::mat4(1.0f);
                    modelMatrix = glm::translate(modelMatrix, glm::vec3(chunkCoordinates[0], chunkCoordinates[1], chunkCoordinates[2]));
                    //update the MVP uniform
                    blockShader.setUniformMat4f("u_modelView", viewMatrix * modelMatrix);
                    m_chunkVertexArrays[meshNum]->bind();
                    mainRenderer.draw(*m_chunkVertexArrays[meshNum], *m_chunkIndexBuffers[meshNum], blockShader);
                    doRenderThreadJobs();
                }
            }
            meshNum++;
        }
        if (m_relableNeeded) {
            relableCompleted = false;
        }
        auto tp2 = std::chrono::high_resolution_clock::now();
        std::cout << "waited " << std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count() << "us for chunks to remesh\n";
    }
    //render water
    waterShader.bind();
    waterShader.setUniformMat4f("u_proj", projMatrix);
    waterShader.setUniform1f("u_renderDistance", m_fogDistance);
    for (unsigned int i = 0; i < m_meshedChunkArrayIndices.size(); i++) {
        if (m_chunkWaterIndexBuffers[i]->getCount() > 0) {
            m_chunks[m_meshedChunkArrayIndices[i]].getChunkPosition(chunkPosition);
            for (unsigned char ii = 0; ii < 3; ii++) {
                chunkCoordinates[ii] = chunkPosition[ii] * constants::CHUNK_SIZE - playerBlockPosition[ii];
            }
            AABB aabb(glm::vec3(chunkCoordinates[0], chunkCoordinates[1], chunkCoordinates[2]), glm::vec3(chunkCoordinates[0] + constants::CHUNK_SIZE, chunkCoordinates[1] + constants::CHUNK_SIZE, chunkCoordinates[2] + constants::CHUNK_SIZE));
            if (aabb.isOnFrustum(viewFrustum)) {
                glm::mat4 modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, glm::vec3(chunkCoordinates[0], chunkCoordinates[1], chunkCoordinates[2]));
                //update the MVP uniform
                waterShader.setUniformMat4f("u_modelView", viewMatrix * modelMatrix);
                m_chunkWaterVertexArrays[i]->bind();
                mainRenderer.draw(*m_chunkWaterVertexArrays[i], *m_chunkWaterIndexBuffers[i], waterShader);
                doRenderThreadJobs();
            }
        }
    }
    m_renderingFrame = false;
}

void world::doRenderThreadJobs() {
    relableChunksIfNeeded();
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
    m_mouseCalls += 1;
    if (m_mouseCalls > 100) {
        processMouseInput();
        m_mouseCalls = 0;
    }
}

void world::updatePlayerPos(float playerX, float playerY, float playerZ) {
    m_newPlayerChunkPosition[0] = floor((playerX) / static_cast<float>(constants::CHUNK_SIZE));
    m_newPlayerChunkPosition[1] = floor((playerY) / static_cast<float>(constants::CHUNK_SIZE));
    m_newPlayerChunkPosition[2] = floor((playerZ) / static_cast<float>(constants::CHUNK_SIZE));
}

void world::relableChunksIfNeeded() {
    if (!m_relableNeeded) {
        bool relableNeeded = !((m_playerChunkPosition[0] == m_newPlayerChunkPosition[0])
            && (m_playerChunkPosition[1] == m_newPlayerChunkPosition[1])
            && (m_playerChunkPosition[2] == m_newPlayerChunkPosition[2]));
        m_relableNeeded = relableNeeded;
        relableCompleted = !m_relableNeeded;
        for (char i = 0; i < 3; i++) {
            m_updatingPlayerChunkPosition[i] = m_newPlayerChunkPosition[i];
        }
    }
    //if the player has moved chunk, update the list of loaded chunks
    if (m_relableNeeded && (!m_renderingFrame)) {
        //wait for all the mesh builder threads to finish their jobs
        bool readyToRelable = false;
        for (char threadNum = 0; threadNum < m_numChunkLoadingThreads; threadNum++) {
            readyToRelable |= !m_threadWaiting[threadNum];
        }
        readyToRelable = !readyToRelable;
        if (!readyToRelable) {
            return;
        }

        unloadAndRelableChunks();
    }
}

unsigned int world::getChunkNumber(int* chunkCoords, int* playerChunkCoords) {
    int adjustedChunkCoords[3];
    for (unsigned char i = 0; i < 3; i++) {
        adjustedChunkCoords[i] = chunkCoords[i] - playerChunkCoords[i] + m_renderDistance;
    }
    
    unsigned int chunkNumber = adjustedChunkCoords[1] * m_renderDiameter * m_renderDiameter;
    chunkNumber += adjustedChunkCoords[2] * m_renderDiameter;
    chunkNumber += adjustedChunkCoords[0];

    return chunkNumber;
}

void world::getChunkCoords(int* chunkCoords, unsigned int chunkNumber, int* playerChunkCoords) {
    int adjustedChunkCoords[3];
    adjustedChunkCoords[0] = chunkNumber % m_renderDiameter;
    adjustedChunkCoords[1] = chunkNumber / (m_renderDiameter * m_renderDiameter);
    adjustedChunkCoords[2] = (chunkNumber - adjustedChunkCoords[1] * m_renderDiameter * m_renderDiameter) / m_renderDiameter;

    for (unsigned char i = 0; i < 3; i++) {
        chunkCoords[i] = adjustedChunkCoords[i] - m_renderDistance + playerChunkCoords[i];
    }
}

void world::loadChunksAroundPlayer(char threadNum) {
    if (m_relableNeeded && (m_numMeshUpdates == 0)) {
        m_threadWaiting[threadNum] = true;
        // locking 
        std::unique_lock<std::mutex> lock(m_relableNeededMtx);
        // waiting 
        m_relableNeededCV.wait(lock, [] { return relableCompleted; });
        m_threadWaiting[threadNum] = false;
    }

    m_accessingArrIndicesVectorsMtx.lock();
    while (m_renderThreadWaitingForArrIndicesVectors) {
        m_accessingArrIndicesVectorsMtx.unlock();
        m_renderThreadWaitingForArrIndicesVectorsMtx.lock();
        m_accessingArrIndicesVectorsMtx.lock();
        m_renderThreadWaitingForArrIndicesVectorsMtx.unlock();
    }
    //find the closest chunk that is not currently loaded
    float minUnloadedChunkDistance = ((m_renderDistance + 1) * (m_renderDistance + 1));
    unsigned int closestUnloadedChunk;
    for (unsigned int i = 0; i < m_numChunks; i++) {
        if ((!(m_loadedChunks[i] || m_loadingChunks[i])) && (m_chunkDistances[i] < minUnloadedChunkDistance)) {
            minUnloadedChunkDistance = m_chunkDistances[i];
            closestUnloadedChunk = i;
        }
    }
    //if there is an unloaded chunk within render distance, find a chunk that is currently unloaded
    if (minUnloadedChunkDistance < ((m_renderDistance + 0.999f) * (m_renderDistance + 0.999f))) {
        unsigned int unloadedChunk = rand() % m_numActualChunks;
        while ((m_chunks[unloadedChunk].inUse)) {
            unloadedChunk++;
            unloadedChunk = unloadedChunk * (unloadedChunk < m_numActualChunks);
        }

        //create the chunk at the position of the unloaded chunk
        int chunkCoords[3];
        getChunkCoords(chunkCoords, closestUnloadedChunk, m_playerChunkPosition);
        m_loadingChunks[closestUnloadedChunk] = true;
        m_chunks[unloadedChunk].inUse = true;
        /*// lock release 
        lock_guard<mutex> lock(m_locatingUnloadedChunkMtx);
        // variable to avoid spurious wakeup 
        locatedUnloadedChunk = true;
        m_locatingUnloadedChunkCV.notify_one();*/
        m_accessingArrIndicesVectorsMtx.unlock();
        loadChunk(unloadedChunk, chunkCoords, threadNum);
        m_loadingChunks[closestUnloadedChunk] = false;
        m_meshedChunksDistance = minUnloadedChunkDistance;
        //build the meshes for any chunks that now have all thier neighbours
        //this is something that can be done at any time, but I am doing it here to increase fluidity of chunk loading
        buildMeshesForNewChunksWithNeighbours(threadNum);
    }
    else {
        m_accessingArrIndicesVectorsMtx.unlock();
        std::this_thread::sleep_for(std::operator""us(100));
        //build the meshes for any chunks that now have all thier neighbours
        //this is something that can be done at any time, but I am doing it here to increase fluidity of chunk loading
        buildMeshesForNewChunksWithNeighbours(threadNum);
    }
}

void world::unloadAndRelableChunks() {
    //unload any meshes and chunks that are out of render distance
    int chunkCoords[3];
    float distance;
    const int maxMicroseconds = 2000;
    auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = startTime;
    while ((std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime).count() < maxMicroseconds)
        && (m_numMeshesUnloaded < (unsigned int)m_meshedChunkArrayIndices.size())) {
        m_chunks[m_meshedChunkArrayIndices[m_numMeshesUnloaded]].getChunkPosition(chunkCoords);
        distance = 0;
        for (unsigned char ii = 0; ii < 3; ii++) {
            distance += (chunkCoords[ii] - m_updatingPlayerChunkPosition[ii]) * (chunkCoords[ii] - m_updatingPlayerChunkPosition[ii]);
        }
        if (distance >= ((m_renderDistance - 0.001f) * (m_renderDistance - 0.001f))) {
            unloadMesh(m_numMeshesUnloaded);
        }
        else {
            m_numMeshesUnloaded++;
        }
        currentTime = std::chrono::high_resolution_clock::now();
    }
    if (m_numMeshesUnloaded == m_meshedChunkArrayIndices.size()) {
        while ((std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime).count() < maxMicroseconds)
            && (m_numChunksUnloaded < (unsigned int)m_unmeshedChunkArrayIndices.size())) {
            m_chunks[m_unmeshedChunkArrayIndices[m_numChunksUnloaded]].getChunkPosition(chunkCoords);
            distance = 0;
            for (unsigned char ii = 0; ii < 3; ii++) {
                distance += (chunkCoords[ii] - m_updatingPlayerChunkPosition[ii]) * (chunkCoords[ii] - m_updatingPlayerChunkPosition[ii]);
            }
            if (distance >= ((m_renderDistance + 0.999f) * (m_renderDistance + 0.999f))) {
                unloadChunk(m_numChunksUnloaded);
            }
            else {
                m_numChunksUnloaded++;
            }
            currentTime = std::chrono::high_resolution_clock::now();
        }
        if (m_numChunksUnloaded == m_unmeshedChunkArrayIndices.size()) {
            m_numMeshesUnloaded = m_numChunksUnloaded = m_numMeshUpdates = 0;

            //update the player's chunk position
            m_playerChunkPosition[0] = m_updatingPlayerChunkPosition[0];
            m_playerChunkPosition[1] = m_updatingPlayerChunkPosition[1];
            m_playerChunkPosition[2] = m_updatingPlayerChunkPosition[2];

            //recalculate which chunks are loaded and unloaded
            for (unsigned int i = 0; i < m_numChunks; i++) {
                m_loadedChunks[i] = false;
                m_meshUpdates[i] = false;
            }
            unsigned int chunkNumber;
            for (unsigned int i = 0; i < m_numActualChunks; i++) {
                if (m_chunks[i].inUse) {
                    m_chunks[i].getChunkPosition(chunkCoords);
                    chunkNumber = getChunkNumber(chunkCoords, m_playerChunkPosition);
                    m_loadedChunks[chunkNumber] = true;
                    m_chunkArrayIndices[chunkNumber] = i;
                }
            }

            //cout << "RELABLE ";

            //clear the list of being meshed chunks as there are no chunks currently being meshed
            //so this is a safe time to do so
            m_beingMeshedChunkArrayIndices.clear();

            relableCompleted = true;
            m_relableNeeded = false;
            // lock release 
            std::lock_guard<std::mutex> lock(m_relableNeededMtx);
            // notify consumer when done
            m_relableNeededCV.notify_all();
        }
    }
}

bool world::chunkHasNeighbours(unsigned int chunkArrayIndex) {
    int chunkPosition[3];
    m_chunks[chunkArrayIndex].getChunkPosition(chunkPosition);
    
    for (unsigned char i = 0; i < 3; i++) {
        if (abs(chunkPosition[i] - m_playerChunkPosition[i]) == m_renderDistance) {
            return false;
        }
    }

    int chunkNumber = getChunkNumber(chunkPosition, m_playerChunkPosition);

    for (unsigned char i = 0; i < 27; i++) {
        if (!(m_loadedChunks[chunkNumber + m_neighbouringChunkIncludingDiaganalOffsets[i]])) {
            return false;
        }
    }

    return true;
}

void world::loadChunk(unsigned int chunkArrayIndex, int* chunkCoords, char threadNum) {
    unsigned int chunkNumber = getChunkNumber(chunkCoords, m_playerChunkPosition);
    m_chunkArrayIndices[chunkNumber] = chunkArrayIndex;

    m_chunks[chunkArrayIndex].recreate(chunkCoords[0], chunkCoords[1], chunkCoords[2]);

    m_accessingArrIndicesVectorsMtx.lock();
    while (m_renderThreadWaitingForArrIndicesVectors) {
        m_accessingArrIndicesVectorsMtx.unlock();
        m_renderThreadWaitingForArrIndicesVectorsMtx.lock();
        m_accessingArrIndicesVectorsMtx.lock();
        m_renderThreadWaitingForArrIndicesVectorsMtx.unlock();
    }
    m_unmeshedChunkArrayIndices.push_back(chunkArrayIndex);
    //set the chunk to be loaded in the array of loaded chunks
    m_loadedChunks[chunkNumber] = true;
    m_accessingArrIndicesVectorsMtx.unlock();
}

void world::unloadChunk(unsigned int chunkVectorIndex) {
    //set the chunk to be unloaded in the array of loaded chunks
    int chunkCoords[3];
    m_chunks[m_unmeshedChunkArrayIndices[chunkVectorIndex]].getChunkPosition(chunkCoords);
    unsigned int chunkNumber = getChunkNumber(chunkCoords, m_playerChunkPosition);
    m_loadedChunks[chunkNumber] = false;

    m_chunks[m_unmeshedChunkArrayIndices[chunkVectorIndex]].unload();

    std::vector<unsigned int>::iterator it = m_unmeshedChunkArrayIndices.begin() + chunkVectorIndex;
    m_unmeshedChunkArrayIndices.erase(it);
}

void world::unloadMesh(unsigned int chunkVectorIndex) {
    m_unmeshedChunkArrayIndices.push_back(m_meshedChunkArrayIndices[chunkVectorIndex]);

    std::vector<unsigned int>::iterator it1 = m_meshedChunkArrayIndices.begin() + chunkVectorIndex;
    m_meshedChunkArrayIndices.erase(it1);

    if (m_chunkIndexBuffers[chunkVectorIndex]->getCount() > 0) {
        m_chunkVertexArrays[chunkVectorIndex]->~vertexArray();
        delete m_chunkVertexArrays[chunkVectorIndex];
        m_chunkVertexBuffers[chunkVectorIndex]->~vertexBuffer();
        delete m_chunkVertexBuffers[chunkVectorIndex];
        m_chunkIndexBuffers[chunkVectorIndex]->~indexBuffer();
        delete m_chunkIndexBuffers[chunkVectorIndex];
    }

    std::vector<vertexArray*>::iterator it2 = m_chunkVertexArrays.begin() + chunkVectorIndex;
    m_chunkVertexArrays.erase(it2);
    std::vector<vertexBuffer*>::iterator it3 = m_chunkVertexBuffers.begin() + chunkVectorIndex;
    m_chunkVertexBuffers.erase(it3);
    std::vector<indexBuffer*>::iterator it4 = m_chunkIndexBuffers.begin() + chunkVectorIndex;
    m_chunkIndexBuffers.erase(it4);

    if (m_chunkWaterIndexBuffers[chunkVectorIndex]->getCount() > 0) {
        m_chunkWaterVertexArrays[chunkVectorIndex]->~vertexArray();
        delete m_chunkWaterVertexArrays[chunkVectorIndex];
        m_chunkWaterVertexBuffers[chunkVectorIndex]->~vertexBuffer();
        delete m_chunkWaterVertexBuffers[chunkVectorIndex];
        m_chunkWaterIndexBuffers[chunkVectorIndex]->~indexBuffer();
        delete m_chunkWaterIndexBuffers[chunkVectorIndex];
    }

    it2 = m_chunkWaterVertexArrays.begin() + chunkVectorIndex;
    m_chunkWaterVertexArrays.erase(it2);
    it3 = m_chunkWaterVertexBuffers.begin() + chunkVectorIndex;
    m_chunkWaterVertexBuffers.erase(it3);
    it4 = m_chunkWaterIndexBuffers.begin() + chunkVectorIndex;
    m_chunkWaterIndexBuffers.erase(it4);
}

void world::addChunkMesh(unsigned int chunkVectorIndex, char threadNum) {
    //set up containers for data for buffers
    m_numChunkVertices[threadNum] = 0;
    m_numChunkIndices[threadNum] = 0;
    m_numChunkWaterVertices[threadNum] = 0;
    m_numChunkWaterIndices[threadNum] = 0;

    //get the position of the chunk
    int chunkPosition[3];
    m_chunks[m_beingMeshedChunkArrayIndices[chunkVectorIndex]].getChunkPosition(chunkPosition);

    //get the chunk number
    unsigned int chunkNumber = getChunkNumber(chunkPosition, m_playerChunkPosition);
    unsigned int testChunkNumber;
    //get the indices of the neighbouring chunks
    unsigned int neighbouringChunkIndices[6];
    for (unsigned char i = 0; i < 6; i++) {
        neighbouringChunkIndices[i] = m_chunkArrayIndices[chunkNumber + m_neighbouringChunkNumberOffets[i]];
    }

    //generate the mesh
    m_chunks[m_beingMeshedChunkArrayIndices[chunkVectorIndex]].buildMesh(m_chunkVertices[threadNum], &m_numChunkVertices[threadNum], m_chunkIndices[threadNum], &m_numChunkIndices[threadNum], m_chunkWaterVertices[threadNum], &m_numChunkWaterVertices[threadNum], m_chunkWaterIndices[threadNum], &m_numChunkWaterIndices[threadNum], neighbouringChunkIndices);

    //if the chunk is empty fill the data will empty values to save interrupting the render thread
    if ((m_numChunkIndices[threadNum] == 0) && (m_numChunkWaterIndices[threadNum] == 0)) {
        m_accessingArrIndicesVectorsMtx.lock();
        while (m_renderThreadWaitingForArrIndicesVectors) {
            m_accessingArrIndicesVectorsMtx.unlock();
            m_renderThreadWaitingForArrIndicesVectorsMtx.lock();
            m_accessingArrIndicesVectorsMtx.lock();
            m_renderThreadWaitingForArrIndicesVectorsMtx.unlock();
        }
        m_chunkVertexArrays.push_back(m_emptyVertexArray);
        m_chunkVertexBuffers.push_back(m_emptyVertexBuffer);
        m_chunkIndexBuffers.push_back(m_emptyIndexBuffer);
        m_chunkWaterVertexArrays.push_back(m_emptyVertexArray);
        m_chunkWaterVertexBuffers.push_back(m_emptyVertexBuffer);
        m_chunkWaterIndexBuffers.push_back(m_emptyIndexBuffer);
        m_meshedChunkArrayIndices.push_back(m_beingMeshedChunkArrayIndices[chunkVectorIndex]);
        m_numMeshUpdates -= m_meshUpdates[chunkNumber];
        m_meshUpdates[chunkNumber] = false;

        m_accessingArrIndicesVectorsMtx.unlock();

        return;
    }
    //wait for the render thread to upload the mesh to the GPU
    m_chunkVectorIndex[threadNum] = chunkVectorIndex;
    m_chunkMeshReady[threadNum] = true;
    chunkMeshUploaded[threadNum] = false;

    // locking
    std::unique_lock<std::mutex> lock(m_chunkMeshReadyMtx[threadNum]);
    // waiting
    while (!chunkMeshUploaded[threadNum]) {
        m_chunkMeshReadyCV[threadNum].wait(lock);
    }
}

void world::uploadChunkMesh(char threadNum) {
    //TODO: precalculate this VBL object
    //create vertex buffer layout
    vertexBufferLayout layout;
    layout.push<float>(3);
    layout.push<float>(2);
    layout.push<float>(1);

    vertexArray *va, *waterVa;
    vertexBuffer *vb, *waterVb;
    indexBuffer *ib, *waterIb;

    if (m_numChunkIndices[threadNum] > 0) {
        //create vertex array
        va = new vertexArray;

        //create vertex buffer
        vb = new vertexBuffer(m_chunkVertices[threadNum], m_numChunkVertices[threadNum] * sizeof(float));

        va->addBuffer(*vb, layout);

        //create index buffer
        ib = new indexBuffer(m_chunkIndices[threadNum], m_numChunkIndices[threadNum]);
    }
    else {
        va = m_emptyVertexArray;
        vb = m_emptyVertexBuffer;
        ib = m_emptyIndexBuffer;
    }

    if (m_numChunkWaterIndices[threadNum] > 0) {
        //create vertex array
        waterVa = new vertexArray;

        //create vertex buffer
        waterVb = new vertexBuffer(m_chunkWaterVertices[threadNum], m_numChunkWaterVertices[threadNum] * sizeof(float));

        waterVa->addBuffer(*waterVb, layout);

        //create index buffer
        waterIb = new indexBuffer(m_chunkWaterIndices[threadNum], m_numChunkWaterIndices[threadNum]);
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
    m_chunkVertexArrays.push_back(va);
    m_chunkVertexBuffers.push_back(vb);
    m_chunkIndexBuffers.push_back(ib);
    m_chunkWaterVertexArrays.push_back(waterVa);
    m_chunkWaterVertexBuffers.push_back(waterVb);
    m_chunkWaterIndexBuffers.push_back(waterIb);
    m_meshedChunkArrayIndices.push_back(m_beingMeshedChunkArrayIndices[m_chunkVectorIndex[threadNum]]);
    int chunkCoords[3];
    m_chunks[m_beingMeshedChunkArrayIndices[m_chunkVectorIndex[threadNum]]].getChunkPosition(chunkCoords);
    unsigned int chunkNumber = getChunkNumber(chunkCoords, m_playerChunkPosition);
    m_numMeshUpdates -= m_meshUpdates[chunkNumber];
    m_meshUpdates[chunkNumber] = false;
    m_accessingArrIndicesVectorsMtx.unlock();
}

void world::buildMeshesForNewChunksWithNeighbours(char threadNum) {
    int numMeshes = 0;
    unsigned int i = 0;
    m_accessingArrIndicesVectorsMtx.lock();
    while (m_renderThreadWaitingForArrIndicesVectors) {
        m_accessingArrIndicesVectorsMtx.unlock();
        m_renderThreadWaitingForArrIndicesVectorsMtx.lock();
        m_accessingArrIndicesVectorsMtx.lock();
        m_renderThreadWaitingForArrIndicesVectorsMtx.unlock();
    }
    while (i < m_unmeshedChunkArrayIndices.size()) {
        if (chunkHasNeighbours(m_unmeshedChunkArrayIndices[i])) {
            unsigned int beingMeshedVectorIndex = m_beingMeshedChunkArrayIndices.size();
            m_beingMeshedChunkArrayIndices.push_back(m_unmeshedChunkArrayIndices[i]);
            std::vector<unsigned int>::iterator it = m_unmeshedChunkArrayIndices.begin() + i;
            m_unmeshedChunkArrayIndices.erase(it);
            m_accessingArrIndicesVectorsMtx.unlock();

            addChunkMesh(beingMeshedVectorIndex, threadNum);

            m_accessingArrIndicesVectorsMtx.lock();
            while (m_renderThreadWaitingForArrIndicesVectors) {
                m_accessingArrIndicesVectorsMtx.unlock();
                m_renderThreadWaitingForArrIndicesVectorsMtx.lock();
                m_accessingArrIndicesVectorsMtx.lock();
                m_renderThreadWaitingForArrIndicesVectorsMtx.unlock();
            }
        }
        else {
            i++;
        }
    }
    m_accessingArrIndicesVectorsMtx.unlock();
}

unsigned char world::shootRay(glm::vec3 startSubBlockPos, int* startBlockPosition, glm::vec3 direction, int* breakBlockCoords, int* placeBlockCoords) {
    //TODO: improve this to make it need less steps
    glm::vec3 rayPos = startSubBlockPos;
    int blockPos[3];
    int steps = 0;
    while (steps < 180) {
        rayPos += direction * 0.025f;
        for (unsigned char ii = 0; ii < 3; ii++) {
            blockPos[ii] = floor(rayPos[ii]) + startBlockPosition[ii];
        }
        short blockType = getBlock(blockPos);
        if ((blockType != 0) && (blockType != 4)) {
            for (unsigned char ii = 0; ii < 3; ii++) {
                breakBlockCoords[ii] = blockPos[ii];
            }

            rayPos -= direction * 0.025f;

            for (unsigned char ii = 0; ii < 3; ii++) {
                placeBlockCoords[ii] = floor(rayPos[ii]) + startBlockPosition[ii];
            }
            return 2;
        }
        steps++;
    }
    return 0;
}

void world::replaceBlock(int* blockCoords, unsigned short blockType) {
    int chunkCoords[3];
    unsigned int blockPosInChunk[3];
    unsigned int blockNumber = constants::CHUNK_SIZE * constants::CHUNK_SIZE * constants::CHUNK_SIZE;
    int testChunkCoords[3];
    for (unsigned char i = 0; i < 3; i++) {
        chunkCoords[i] = floor(static_cast<float>(blockCoords[i]) / constants::CHUNK_SIZE);
        blockPosInChunk[i] = blockCoords[i] - chunkCoords[i] * constants::CHUNK_SIZE;
        testChunkCoords[i] = chunkCoords[i] + 1;
    }
    unsigned int i = 0;
    m_accessingArrIndicesVectorsMtx.lock();
    while (m_renderThreadWaitingForArrIndicesVectors) {
        m_accessingArrIndicesVectorsMtx.unlock();
        m_renderThreadWaitingForArrIndicesVectorsMtx.lock();
        m_accessingArrIndicesVectorsMtx.lock();
        m_renderThreadWaitingForArrIndicesVectorsMtx.unlock();
    }

    while ((!((testChunkCoords[0] == chunkCoords[0]) && (testChunkCoords[1] == chunkCoords[1]) && (testChunkCoords[2] == chunkCoords[2]))) && (i < m_meshedChunkArrayIndices.size())) {
        m_chunks[m_meshedChunkArrayIndices[i]].getChunkPosition(testChunkCoords);
        i++;
    }
    if ((testChunkCoords[0] == chunkCoords[0]) && (testChunkCoords[1] == chunkCoords[1]) && (testChunkCoords[2] == chunkCoords[2])) {
        blockNumber = m_chunks[m_meshedChunkArrayIndices[i - 1]].getBlockNumber(blockPosInChunk);
        std::cout << (unsigned int)m_chunks[m_meshedChunkArrayIndices[i - 1]].getSkyLight(blockNumber) << " light level\n";
        m_chunks[m_meshedChunkArrayIndices[i - 1]].setBlock(blockNumber, blockType);

        std::vector<unsigned int> relitChunks;
        auto tp1 = std::chrono::high_resolution_clock::now();
        relightChunksAroundBlock(blockCoords, &relitChunks);
        auto tp2 = std::chrono::high_resolution_clock::now();
        std::cout << "relight took " << std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count() << "us\n";
        
        
        tp1 = std::chrono::high_resolution_clock::now();
        for (unsigned char reloadedChunk = 0; reloadedChunk < relitChunks.size(); reloadedChunk++) {
            getChunkCoords(chunkCoords, (relitChunks[reloadedChunk]), m_playerChunkPosition);
            int i = 0;
            while (i < m_meshedChunkArrayIndices.size()) {
                m_chunks[m_meshedChunkArrayIndices[i]].getChunkPosition(testChunkCoords);
                if ((testChunkCoords[0] == chunkCoords[0]) && (testChunkCoords[1] == chunkCoords[1]) && (testChunkCoords[2] == chunkCoords[2])) {
                    unsigned int chunkNum = getChunkNumber(testChunkCoords, m_playerChunkPosition);
                    m_numMeshUpdates += 1 - m_meshUpdates[chunkNum];
                    m_meshUpdates[chunkNum] = true;
                    unloadMesh(i);
                }
                i++;
            }
        }
        tp2 = std::chrono::high_resolution_clock::now();
        std::cout << "unmesh took " << std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count() << "us\n";
    }
    /* Below is code to allow for replacing blocks in unmeshed chunks
    else {
        i = 0;
        while ((!((testChunkCoords[0] == chunkCoords[0]) && (testChunkCoords[1] == chunkCoords[1]) && (testChunkCoords[2] == chunkCoords[2]))) && (i < m_beingMeshedChunkArrayIndices.size())) {
            m_chunks[m_beingMeshedChunkArrayIndices[i]].getChunkPosition(testChunkCoords);
            i++;
        }
        if ((testChunkCoords[0] == chunkCoords[0]) && (testChunkCoords[1] == chunkCoords[1]) && (testChunkCoords[2] == chunkCoords[2])) {
            blockNumber = m_chunks[m_beingMeshedChunkArrayIndices[i - 1]].getBlockNumber(blockPosInChunk);
            m_chunks[m_beingMeshedChunkArrayIndices[i - 1]].setBlock(blockNumber, blockType);
        }
        else {
            i = 0;
            while (!((testChunkCoords[0] == chunkCoords[0]) && (testChunkCoords[1] == chunkCoords[1]) && (testChunkCoords[2] == chunkCoords[2])) && (i < m_unmeshedChunkArrayIndices.size())) {
                m_chunks[m_unmeshedChunkArrayIndices[i]].getChunkPosition(testChunkCoords);
                i++;
            }
            if ((testChunkCoords[0] == chunkCoords[0]) && (testChunkCoords[1] == chunkCoords[1]) && (testChunkCoords[2] == chunkCoords[2])) {
                blockNumber = m_chunks[m_unmeshedChunkArrayIndices[i - 1]].getBlockNumber(blockPosInChunk);
                m_chunks[m_unmeshedChunkArrayIndices[i - 1]].setBlock(blockNumber, blockType);
            }
        }
    }*/
    m_accessingArrIndicesVectorsMtx.unlock();

    if (m_relableNeeded) {
        //release the chunk loader threads so that the required chunks can be remeshed
        relableCompleted = true;
        // lock release 
        std::lock_guard<std::mutex> lock(m_relableNeededMtx);
        // notify consumer when done
        m_relableNeededCV.notify_all();
    }
}

unsigned short world::getBlock(int* blockCoords) {
    int chunkCoords[3];
    unsigned int blockPosInChunk[3];
    for (unsigned char i = 0; i < 3; i++) {
        chunkCoords[i] = floor(static_cast<float>(blockCoords[i]) / constants::CHUNK_SIZE);
        blockPosInChunk[i] = blockCoords[i] - chunkCoords[i] * constants::CHUNK_SIZE;
    }

    unsigned int chunkNumber = getChunkNumber(chunkCoords, m_playerChunkPosition);

    if (!(m_loadedChunks[chunkNumber])) {
        return 0;
    }

    unsigned int blockNumber = m_chunks[m_chunkArrayIndices[chunkNumber]].getBlockNumber(blockPosInChunk);

    return m_chunks[m_chunkArrayIndices[chunkNumber]].getBlock(blockNumber);
}

worldInfo world::getWorldInfo() {
    return m_worldInfo;
}

char world::getNumChunkLoaderThreads() {
    return m_numChunkLoadingThreads;
}

void world::processMouseInput() {
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
        if ((abs(localCursorPosition[0] - m_windowDimensions[0] / 2) > m_windowDimensions[0] / 16)
            || (abs(localCursorPosition[1] - m_windowDimensions[1] / 2) > m_windowDimensions[1] / 16)) {
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

void world::setMouseData(double* lastMousePoll,
    bool* playing,
    bool* lastPlaying,
    float* yaw,
    float* pitch,
    int* lastMousePos,
    camera* viewCamera,
    SDL_Window* window,
    int* windowDimensions) {
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

void world::initPlayerPos(float playerX, float playerY, float playerZ) {
    m_playerChunkPosition[0] = m_newPlayerChunkPosition[0] = m_updatingPlayerChunkPosition[0] = floor((playerX) / static_cast<float>(constants::CHUNK_SIZE));
    m_playerChunkPosition[1] = m_newPlayerChunkPosition[1] = m_updatingPlayerChunkPosition[1] = floor((playerY) / static_cast<float>(constants::CHUNK_SIZE));
    m_playerChunkPosition[2] = m_newPlayerChunkPosition[2] = m_updatingPlayerChunkPosition[2] = floor((playerZ) / static_cast<float>(constants::CHUNK_SIZE));
    m_relableNeeded = true;
}

void world::relightChunksAroundBlock(const int* blockCoords, std::vector<unsigned int>* relitChunks) {
    //find the lowest block in the column that is loaded
    int lowestChunkInWorld = m_playerChunkPosition[1] - m_renderDistance;
    int chunkCoords[3] = { 0, 0, 0 };
    for (char i = 0; i < 3; i++) {
        chunkCoords[i] = floor(static_cast<float>(blockCoords[i]) / constants::CHUNK_SIZE);
    }
    unsigned int chunkNum = getChunkNumber(chunkCoords, m_playerChunkPosition);
    while ((chunkCoords[1] > lowestChunkInWorld) && (m_loadedChunks[chunkNum])) {
        chunkCoords[1] -= 1;
        chunkNum = getChunkNumber(chunkCoords, m_playerChunkPosition);
    }
    int lowestLoadedBlockInColumn = (chunkCoords[1] + 1) * constants::CHUNK_SIZE;
    //find the lowest block in the column that has a full sky access
    int blockPos[3];
    for (char i = 0; i < 3; i++) {
        blockPos[i] = blockCoords[i];
    }
    blockPos[1]--;
    while (blockPos[1] - constants::skyLightMaxValue + 1 >= lowestLoadedBlockInColumn) {
        unsigned char blockType = getBlock(blockPos);
        if (constants::dimsLight[blockType] || (!constants::transparent[blockType])) {
            blockPos[1]--;
            break;
        }
        blockPos[1]--;
    }
    int lowestFullySkylitBlockInColumn = blockPos[1] + 2;

    //find the furthest blocks that the skylight could spread to and add them to a vector
    std::vector<int> blockCoordsToBeRelit;
    int chunkLayerHeight = floor(static_cast<float>(blockCoords[1] + constants::skyLightMaxValue - 1) / constants::CHUNK_SIZE) * constants::CHUNK_SIZE;
    while (chunkLayerHeight >= lowestFullySkylitBlockInColumn - constants::skyLightMaxValue + 1 - constants::CHUNK_SIZE) {
        blockPos[0] = blockCoords[0];
        blockPos[1] = chunkLayerHeight;
        blockPos[2] = blockCoords[2];
        for (char i = 0; i < 3; i++) {
            blockCoordsToBeRelit.push_back(blockPos[i]);
        }
        blockPos[0] += constants::skyLightMaxValue - 1;
        for (char i = 0; i < constants::skyLightMaxValue - 1; i++) {
            blockPos[0]--;
            blockPos[2]++;
            for (char i = 0; i < 3; i++) {
                blockCoordsToBeRelit.push_back(blockPos[i]);
            }
        }
        for (char i = 0; i < constants::skyLightMaxValue - 1; i++) {
            blockPos[0]--;
            blockPos[2]--;
            for (char i = 0; i < 3; i++) {
                blockCoordsToBeRelit.push_back(blockPos[i]);
            }
        }
        for (char i = 0; i < constants::skyLightMaxValue - 1; i++) {
            blockPos[0]++;
            blockPos[2]--;
            for (char i = 0; i < 3; i++) {
                blockCoordsToBeRelit.push_back(blockPos[i]);
            }
        }
        for (char i = 0; i < constants::skyLightMaxValue - 1; i++) {
            blockPos[0]++;
            blockPos[2]++;
            for (char i = 0; i < 3; i++) {
                blockCoordsToBeRelit.push_back(blockPos[i]);
            }
        }
        chunkLayerHeight -= constants::CHUNK_SIZE;
    }

    //for every block that the skylight could spread to, add that chunk to a vector only if it is not there already
    std::vector<unsigned int> chunksToBeRelit;
    for (int i = 0; i < blockCoordsToBeRelit.size(); i += 3) {
        chunkCoords[0] = floor(static_cast<float>(blockCoordsToBeRelit[i]) / constants::CHUNK_SIZE);
        chunkCoords[1] = blockCoordsToBeRelit[i + 1] / constants::CHUNK_SIZE;
        chunkCoords[2] = floor(static_cast<float>(blockCoordsToBeRelit[i + 2]) / constants::CHUNK_SIZE);
        chunkNum = getChunkNumber(chunkCoords, m_playerChunkPosition);
        if (std::find(chunksToBeRelit.begin(), chunksToBeRelit.end(), chunkNum) == chunksToBeRelit.end()) {
            chunksToBeRelit.push_back(chunkNum);
            m_chunks[m_chunkArrayIndices[chunkNum]].clearSkyLight();
        }
    }

    int numChunksRelit = 0;
    int numSpreads = 0;
    while (chunksToBeRelit.size() > 0) {
        bool neighbouringChunksToRelight[6];
        unsigned int neighbouringChunkIndices[6];
        unsigned int neighbouringChunkNumbers[6];
        bool neighbouringChunksLoaded = true;
        //check that the chunk has its neighbours loaded so that it can be lit
        for (char ii = 0; ii < 6; ii++) {
            if ((((int)(chunksToBeRelit[0]) + m_neighbouringChunkNumberOffets[ii]) < 0) 
                || (((int)(chunksToBeRelit[0]) + m_neighbouringChunkNumberOffets[ii]) > m_renderDiameter * m_renderDiameter * m_renderDiameter)) {
                neighbouringChunksLoaded = false;
                break;
            }
            neighbouringChunkNumbers[ii] = chunksToBeRelit[0] + m_neighbouringChunkNumberOffets[ii];
            if (!m_loadedChunks[neighbouringChunkNumbers[ii]]) {
                neighbouringChunksLoaded = false;
                break;
            }
            neighbouringChunkIndices[ii] = m_chunkArrayIndices[neighbouringChunkNumbers[ii]];
        }
        //if the chunk's neighbours aren't loaded, remove the chunk as it cannot be lit correctly
        if (!neighbouringChunksLoaded) {
            m_chunks[m_chunkArrayIndices[chunksToBeRelit[0]]].setSkyLightToBeOutdated();
            std::vector<unsigned int>::iterator it = chunksToBeRelit.begin();
            chunksToBeRelit.erase(it);
            continue;
        }
        //clear the skylight data for the chunk and relight it
        //m_chunks[m_chunkArrayIndices[chunksToBeRelit[0]]].clearSkyLight();
        m_chunks[m_chunkArrayIndices[chunksToBeRelit[0]]].calculateSkyLight(neighbouringChunkIndices, neighbouringChunksToRelight);
        if (std::find(relitChunks->begin(), relitChunks->end(), chunksToBeRelit[0]) == relitChunks->end()) {
            relitChunks->push_back(chunksToBeRelit[0]);
        }
        std::vector<unsigned int>::iterator it = chunksToBeRelit.begin();
        chunksToBeRelit.erase(it);
        //add the neighbouring chunks that were marked as needing recalculating to the queue
        for (char i = 0; i < 6; i++) {
            if (!neighbouringChunksToRelight[i]) {
                continue;
            }
            if (std::find(chunksToBeRelit.begin(), chunksToBeRelit.end(), neighbouringChunkNumbers[i]) == chunksToBeRelit.end()) {
                chunksToBeRelit.push_back(neighbouringChunkNumbers[i]);
                numSpreads++;
            }
        }
        numChunksRelit++;
    }
    std::cout << numChunksRelit << " chunks relit with " << numSpreads << " spreads\n";
}