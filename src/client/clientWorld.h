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

#pragma once

#include "core/pch.h"

#include "client/graphics/renderer.h"
#include "client/graphics/vertexBuffer.h"
#include "client/graphics/indexBuffer.h"
#include "client/graphics/vertexArray.h"
#include "client/graphics/shader.h"
#include "client/graphics/texture.h"
#include "client/graphics/camera.h"
#include "core/chunk.h"
#include "core/entities/meshManager.h"
#include "core/packet.h"
#include "core/utils/iVec3.h"
#include "core/serverWorld.h"

#include <SDL2/SDL.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "enet/enet.h"

namespace client {

struct MeshData {
    VertexArray* vertexArray;
    VertexBuffer* vertexBuffer;
    IndexBuffer* indexBuffer;
    VertexArray* waterVertexArray;
    VertexBuffer* waterVertexBuffer;
    IndexBuffer* waterIndexBuffer;
};

class ClientWorld {
private:
    static const inline std::array<IVec3, 6> s_neighbouringChunkOffsets = {
        IVec3(0, -1, 0),
        IVec3(0, 0, -1),
        IVec3(-1, 0, 0),
        IVec3(1, 0, 0),
        IVec3(0, 0, 1),
        IVec3(0, 1, 0)
    };

    bool m_singleplayer;
    uint16_t m_renderDistance;
    uint16_t m_renderDiameter;
    int m_playerChunkPosition[3];
    int m_newPlayerChunkPosition[3];
    int m_updatingPlayerChunkPosition[3];
    IVec3 m_neighbouringChunkIncludingDiaganalOffsets[27];
    uint16_t m_numChunkLoadingThreads;
    bool m_renderingFrame;
    float m_meshedChunksDistance;
    float m_fogDistance;
    double m_timeByDTs;
    uint64_t m_seed;

    //mouse polling info
    std::chrono::time_point<std::chrono::_V2::steady_clock, std::chrono::_V2::steady_clock::duration> m_startTime;
    double* m_lastMousePoll;
    bool* m_playing;
    bool* m_lastPlaying;
    float* m_yaw;
    float* m_pitch;
    int* m_lastMousePos;
    Camera* m_viewCamera;
    SDL_Window* m_window;
    uint32_t* m_windowDimensions;

    std::unordered_map<IVec3, MeshData> m_meshes;
    VertexBuffer* m_emptyVertexBuffer;
    IndexBuffer* m_emptyIndexBuffer;
    VertexArray* m_emptyVertexArray;
    std::unordered_set<IVec3> m_unmeshedChunks;
    std::unordered_set<IVec3> m_beingMeshesdChunks;
    std::unordered_set<IVec3> m_meshUpdates; //stores chunks that have to have their meshes rebuilt after a block update
    std::unordered_set<IVec3> m_meshesToUpdate;
    std::queue<IVec3> m_recentChunksBuilt;
    //mesh building data - this is stored at class-level because it allows it to be
    //accessed from multiple threads
    uint32_t* m_numChunkVertices; //array to allow for each mesh-building thread to have its own value
    uint32_t* m_numChunkWaterVertices; //array to allow for each mesh-building thread to have its own value
    uint32_t* m_numChunkIndices; //array to allow for each mesh-building thread to have its own value
    uint32_t* m_numChunkWaterIndices; //array to allow for each mesh-building thread to have its own value
    IVec3* m_chunkPosition; //array to allow for each mesh-building thread to have its own value
    float** m_chunkVertices; //2d array to allow for each mesh-building thread to have its own array
    uint32_t** m_chunkIndices; //2d array to allow for each mesh-building thread to have its own array
    float** m_chunkWaterVertices; //2d array to allow for each mesh-building thread to have its own array
    uint32_t** m_chunkWaterIndices; //2d array to allow for each mesh-building thread to have its own array

    //communication
    std::mutex* m_chunkMeshReadyMtx;
    std::condition_variable* m_chunkMeshReadyCV;
    std::mutex m_unmeshNeededMtx;
    std::condition_variable m_unmeshNeededCV;
    std::mutex m_accessingArrIndicesVectorsMtx;
    std::mutex m_renderThreadWaitingForArrIndicesVectorsMtx;
    std::mutex m_unmeshedChunksMtx;
    bool m_renderThreadWaitingForArrIndicesVectors;
    bool* m_chunkMeshReady;
    bool m_unmeshNeeded;
    bool* m_unmeshOccurred;
    bool* m_threadWaiting;
    int m_mouseCalls;
    int m_numRelights;

    ServerWorld<true> m_integratedServer;
    int m_clientID;
    MeshManager<true> m_meshManager;
    std::unique_ptr<VertexArray> m_entityVertexArray;
    std::unique_ptr<VertexBuffer> m_entityVertexBuffer;
    std::unique_ptr<IndexBuffer> m_entityIndexBuffer;

    void unloadMesh(const IVec3& chunkPosition);
    bool chunkHasNeighbours(const IVec3& chunkPosition);
    // Adds chunks to the vector if the modified block is in or bordering the chunk
    void addChunksToRemesh(std::vector<IVec3>& chunksToRemesh, const IVec3& modifiedBlockPos,
        const IVec3& modifiedBlockChunk);
    void addChunkMesh(const IVec3& chunkPosition, int8_t threadNum);
    void uploadChunkMesh(int8_t threadNum);
    void unmeshChunks();

public:
    ClientWorld(uint16_t renderDistance, uint64_t seed, bool singleplayer, const IVec3& playerPos);
    void renderWorld(Renderer mainRenderer, Shader& blockShader, Shader& waterShader, glm::mat4
        viewMatrix, glm::mat4 projMatrix, int* playerBlockPosition, float aspectRatio, float fov,
        float skyLightIntensity, double DT);
    void loadChunksAroundPlayerSingleplayer(int8_t threadNum);
    void loadChunksAroundPlayerMultiplayer(int8_t threadNum);
    void buildMeshesForNewChunksWithNeighbours(int8_t threadNum);
    uint8_t shootRay(glm::vec3 startSubBlockPos, int* startBlockPosition, glm::vec3 direction, int* breakBlockCoords, int* placeBlockCoords);
    void replaceBlock(const IVec3& blockCoords, uint8_t blockType);
    inline uint8_t getBlock(int* blockCoords) {
        return m_integratedServer.getBlock(IVec3(blockCoords));
    }
    inline int getRenderDistance() {
        return m_renderDistance;
    }
    inline int8_t getNumChunkLoaderThreads() {
        return m_numChunkLoadingThreads;
    }
    void doRenderThreadJobs();
    void updateMeshes();
    void updatePlayerPos(float playerX, float playerY, float playerZ);
    void initialiseEntityRenderBuffers();
    void setMouseData(double* lastMousePoll,
                      bool* playing,
                      bool* lastPlaying,
                      float* yaw,
                      float* pitch,
                      int* lastMousePos,
                      Camera* viewCamera,
                      SDL_Window* window,
                      uint32_t* windowDimensions);
    void processMouseInput();
    inline void setClientID(int ID) {
        m_clientID = ID;
    }
    inline int getClientID() {
        return m_clientID;
    }
    void loadChunkFromPacket(Packet<uint8_t, 9 * constants::CHUNK_SIZE *
        constants::CHUNK_SIZE * constants::CHUNK_SIZE>& payload);
    inline void tick() {
        m_integratedServer.tick();
    }
    inline uint32_t getTickNum() {
        return m_integratedServer.getTickNum();
    }
    inline bool isSinglePlayer() {
        return m_singleplayer;
    }
    inline ResourcePack& getResourcePack() {
        return m_integratedServer.getResourcePack();
    }
    void setThreadWaiting(uint8_t threadNum, bool value);
};

}  // namespace client
