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

#include "client/graphics/renderer.h"
#include "client/graphics/vertexBuffer.h"
#include "client/graphics/indexBuffer.h"
#include "client/graphics/vertexArray.h"
#include "client/graphics/shader.h"
#include "client/graphics/texture.h"
#include "client/graphics/camera.h"
#include "core/chunk.h"
#include "core/packet.h"
#include "core/position.h"
#include "core/serverWorld.h"

#include "core/pch.h"

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
	static const inline std::array<Position, 6> s_neighbouringChunkOffsets = { Position(0, -1, 0),
        Position(0, 0, -1),
        Position(-1, 0, 0),
        Position(1, 0, 0),
        Position(0, 0, 1),
        Position(0, 1, 0) };

	bool m_singleplayer;
	unsigned short m_renderDistance;
	unsigned short m_renderDiameter;
	int m_playerChunkPosition[3];
	int m_newPlayerChunkPosition[3];
	int m_updatingPlayerChunkPosition[3];
	Position m_neighbouringChunkIncludingDiaganalOffsets[27];
	unsigned short m_numChunkLoadingThreads;
	bool m_renderingFrame;
	float m_meshedChunksDistance;
	float m_fogDistance;
	double m_timeByDTs;
	unsigned long long m_seed;

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
	unsigned int* m_windowDimensions;

	std::unordered_map<Position, MeshData> m_meshes;
	VertexBuffer* m_emptyVertexBuffer;
	IndexBuffer* m_emptyIndexBuffer;
	VertexArray* m_emptyVertexArray;
	std::unordered_set<Position> m_unmeshedChunks;
	std::unordered_set<Position> m_beingMeshesdChunks;
	std::unordered_set<Position> m_meshUpdates; //stores chunks that have to have their meshes rebuilt after a block update
	std::unordered_set<Position> m_meshesToUpdate;
	std::queue<Position> m_recentChunksBuilt;
	//mesh building data - this is stored at class-level because it allows it to be
	//accessed from multiple threads
	unsigned int* m_numChunkVertices; //array to allow for each mesh-building thread to have its own value
	unsigned int* m_numChunkWaterVertices; //array to allow for each mesh-building thread to have its own value
	unsigned int* m_numChunkIndices; //array to allow for each mesh-building thread to have its own value
	unsigned int* m_numChunkWaterIndices; //array to allow for each mesh-building thread to have its own value
	Position* m_chunkPosition; //array to allow for each mesh-building thread to have its own value
	float** m_chunkVertices; //2d array to allow for each mesh-building thread to have its own array
	unsigned int** m_chunkIndices; //2d array to allow for each mesh-building thread to have its own array
	float** m_chunkWaterVertices; //2d array to allow for each mesh-building thread to have its own array
	unsigned int** m_chunkWaterIndices; //2d array to allow for each mesh-building thread to have its own array

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

	void unloadMesh(const Position& chunkPosition);
	bool chunkHasNeighbours(const Position& chunkPosition);
	// Adds chunks to the vector if the modified block is in or bordering the chunk
	void addChunksToRemesh(std::vector<Position>& chunksToRemesh, const Position& modifiedBlockPos,
		const Position& modifiedBlockChunk);
	void addChunkMesh(const Position& chunkPosition, char threadNum);
	void uploadChunkMesh(char threadNum);
	void unmeshChunks();

public:
	ClientWorld(unsigned short renderDistance, unsigned long long seed, bool singleplayer, const Position& playerPos);
	void renderChunks(Renderer mainRenderer, Shader& blockShader, Shader& waterShader, glm::mat4
		viewMatrix, glm::mat4 projMatrix, int* playerBlockPosition, float aspectRatio, float fov,
		float skyLightIntensity, double DT);
	void loadChunksAroundPlayerSingleplayer(char threadNum);
	void loadChunksAroundPlayerMultiplayer(char threadNum);
	void buildMeshesForNewChunksWithNeighbours(char threadNum);
	unsigned char shootRay(glm::vec3 startSubBlockPos, int* startBlockPosition, glm::vec3 direction, int* breakBlockCoords, int* placeBlockCoords);
	void replaceBlock(const Position& blockCoords, unsigned char blockType);
	inline unsigned char getBlock(int* blockCoords) {
		return m_integratedServer.getBlock(Position(blockCoords));
	}
	inline int getRenderDistance() {
		return m_renderDistance;
	}
	inline char getNumChunkLoaderThreads() {
		return m_numChunkLoadingThreads;
	}
	void doRenderThreadJobs();
	void updateMeshes();
	void updatePlayerPos(float playerX, float playerY, float playerZ);
	void setMouseData(double* lastMousePoll,
					  bool* playing,
					  bool* lastPlaying,
					  float* yaw,
					  float* pitch,
					  int* lastMousePos,
					  Camera* viewCamera,
					  SDL_Window* window,
					  unsigned int* windowDimensions);
	void processMouseInput();
	inline void setClientID(int ID) {
		m_clientID = ID;
	}
	inline int getClientID() {
		return m_clientID;
	}
	void loadChunkFromPacket(Packet<unsigned char, 9 * constants::CHUNK_SIZE *
        constants::CHUNK_SIZE * constants::CHUNK_SIZE>& payload);
	inline void tick() {
		m_integratedServer.tick();
	}
    inline unsigned int getTickNum() {
        return m_integratedServer.getTickNum();
    }
	inline bool isSinglePlayer() {
		return m_singleplayer;
	}
    inline ResourcePack& getResourcePack() {
        return m_integratedServer.getResourcePack();
    }
	void setThreadWaiting(unsigned char threadNum, bool value);
};

}  // namespace client