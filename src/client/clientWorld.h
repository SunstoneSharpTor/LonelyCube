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

#pragma once

#include "core/pch.h"

#include "enet/enet.h"

#include "client/graphics/camera.h"
#include "client/graphics/entityMeshManager.h"
#include "client/graphics/renderer.h"
#include "core/packet.h"
#include "core/serverWorld.h"
#include "core/utils/iVec3.h"
#include <atomic>
#include <condition_variable>

namespace lonelycube::client {

struct MeshData {
    IVec3 chunkPosition;
    GPUMeshBuffers blockMesh;
    GPUMeshBuffers waterMesh;
};

class ClientWorld {
public:
    ServerWorld<true> integratedServer;

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
    int m_renderDistance;
    int m_renderDiameter;
    int m_playerChunkPosition[3];
    int m_newPlayerChunkPosition[3];
    int m_updatingPlayerChunkPosition[3];
    IVec3 m_neighbouringChunkIncludingDiaganalOffsets[27];
    uint16_t m_numChunkLoadingThreads;
    bool m_renderingFrame;
    float m_meshedChunksDistance;
    float m_fogDistance;
    double m_timeByDTs;
    Camera m_viewCamera;
    Renderer& m_renderer;

    std::unordered_map<IVec3, std::size_t> m_meshArrayIndices;
    std::vector<MeshData> m_meshes;
    std::unordered_set<IVec3> m_unmeshedChunks;
    std::unordered_set<IVec3> m_beingMeshesdChunks;
    std::unordered_set<IVec3> m_meshUpdates; //stores chunks that have to have their meshes rebuilt after a block update
    std::unordered_set<IVec3> m_meshesToUpdate;
    std::array<std::vector<MeshData>, VulkanEngine::MAX_FRAMES_IN_FLIGHT> m_meshesToUnload;
    std::deque<IVec3> m_recentChunksBuilt;
    // Mesh building data - this is stored at class-level because it allows it to be
    // accessed from multiple threads
    // Vectors to allow for each mesh-building thread to have its own value
    std::vector<IVec3> m_chunkPosition;
    std::vector<std::vector<float>> m_chunkVertices;
    std::vector<std::vector<uint32_t>> m_chunkIndices;
    std::vector<std::vector<float>> m_chunkWaterVertices;
    std::vector<std::vector<uint32_t>> m_chunkWaterIndices;

    //communication
    std::unique_ptr<std::mutex[]> m_chunkMeshReadyMtx;
    std::unique_ptr<std::condition_variable[]> m_chunkMeshReadyCV;
    std::mutex m_unmeshNeededMtx;
    std::condition_variable m_unmeshNeededCV;
    std::mutex m_meshesToUpdateMtx;
    std::mutex m_meshUpdatesMtx;
    std::mutex m_renderThreadWaitingForMeshUpdatesMtx;
    std::mutex m_unmeshedChunksMtx;
    bool m_renderThreadWaitingForMeshUpdates;
    std::vector<bool> m_chunkMeshReady;
    bool m_unmeshNeeded;
    bool m_readyForChunkUnload;
    bool m_unloadingChunks;
    std::condition_variable m_readyForChunkUnloadCV;
    std::mutex m_readyForChunkUnloadMtx;
    std::vector<bool> m_threadWaiting;
    int m_numRelights;

    ENetPeer* m_peer;
    std::mutex& m_networkingMtx;
    int m_clientID;
    bool m_chunkRequestScheduled;

    EntityMeshManager m_entityMeshManager;
    std::vector<GPUDynamicMeshBuffers> m_entityMeshes;

    void unloadMesh(MeshData& mesh);
    bool chunkHasNeighbours(const IVec3& chunkPosition);
    // Adds chunks to the vector if the modified block is in or bordering the chunk
    void addChunksToRemesh(std::vector<IVec3>& chunksToRemesh, const IVec3& modifiedBlockPos,
        const IVec3& modifiedBlockChunk);
    void addChunkMesh(const IVec3& chunkPosition, int threadNum);
    void uploadChunkMesh(int threadNum);
    void unmeshChunks();
    void unloadMeshes();
    void waitIfMeshesNeedUnloading(int threadNum);

public:
    ClientWorld(
        int renderDistance, uint64_t seed, bool singleplayer, const IVec3& playerPos,
        ENetPeer* peer, std::mutex& networkingMutex, Renderer& renderer
    );
    void renderWorld(
        const glm::mat4& viewProj, const int* playerBlockPos, const glm::vec3& playerSubBlockPos,
        float aspectRatio, float fov, float skyLightIntensity, double DT
    );
    void loadChunksAroundPlayerSingleplayer(int threadNum);
    bool loadChunksAroundPlayerMultiplayer(int threadNum);
    bool buildMeshesForNewChunksWithNeighbours(int threadNum);
    uint8_t shootRay(glm::vec3 startSubBlockPos, int* startBlockPosition, glm::vec3 direction, int* breakBlockCoords, int* placeBlockCoords);
    void replaceBlock(const IVec3& blockCoords, uint8_t blockType);
    inline int getRenderDistance() {
        return m_renderDistance;
    }
    inline int getNumChunkLoaderThreads() {
        return m_numChunkLoadingThreads;
    }
    void doRenderThreadJobs();
    void updateMeshes();
    void updatePlayerPos(IVec3 playerBlockCoords, Vec3 playerSubBlockCoords);
    void unloadOutOfRangeMeshesIfNeeded();
    void unloadAllMeshes();
    void buildEntityMesh(const IVec3& playerBlockPos);
    void freeEntityMeshes();
    inline void setClientID(int ID) {
        m_clientID = ID;
    }
    inline int getClientID() {
        return m_clientID;
    }
    void loadChunkFromPacket(Packet<uint8_t, 9 * constants::CHUNK_SIZE *
        constants::CHUNK_SIZE * constants::CHUNK_SIZE>& payload);
    inline bool isSinglePlayer() {
        return m_singleplayer;
    }
    void setThreadWaiting(int threadNum, bool value);
    inline void updateViewCamera(const Camera& camera)
    {
        m_viewCamera = camera;
    }
    void requestMoreChunks();
    inline void scheduleChunkRequest()
    {
        m_chunkRequestScheduled = true;
    }
};

}  // namespace lonelycube::client
